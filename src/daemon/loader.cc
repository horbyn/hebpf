// clang-format off
#include "loader.h"
#include <filesystem>
#include <fstream>
#include "src/common/assert.h"
#include "src/common/common.h"
#include "src/common/exception.h"
#include "src/inotify/inotify_manager.h"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace daemon {

Loader::~Loader() { clear(); }

/**
 * @brief 加载 eBPF 程序
 *
 * @param so_path eBPF 程序动态库路径（必须是 so 动态库）
 * @param hook_type hook 类型
 * @param ifindex 网卡索引
 * @param config_path eBPF 程序配置路径
 * @return bool 加载成功返回 true，加载失败返回 false
 */
bool Loader::loadService(std::string_view so_path, HookType hook_type, int ifindex,
                         std::string_view config_path) {
  ASSERT(!so_path.empty());
  ASSERT(hook_type != HookType::UNKNOWN);
  ASSERT(ifindex > 0);

  auto real_so_path = std::string{so_path};
  // Scheduler 动态库格式为 /path/to/libhebpf_scheduler.so;xdpgen_11
  //   其余动态库格式为 /path/to/libhebpf_lb.so
  //   所以这里需要拆分动态库路径
  auto pos = so_path.find_last_of(';');
  if (pos != std::string::npos) {
    real_so_path = so_path.substr(0, pos);
  }

  std::unique_ptr<void, DlCloser> handle(dlopen(real_so_path.c_str(), RTLD_LAZY | RTLD_LOCAL));
  if (handle == nullptr) {
    throw EXCEPT(fmt::format("dlopen() failed: {}", dlerror()));
  }

  auto create_fn = reinterpret_cast<CreateServiceFn>(dlsym(handle.get(), SERVICE_CREATE.data()));
  if (create_fn == nullptr) {
    throw EXCEPT(fmt::format("Failed to get factory symbols from {}", real_so_path));
  }

  auto temp_instance = create_fn();
  if (temp_instance == nullptr) {
    throw EXCEPT(fmt::format("Exported function from {} returned nullptr", real_so_path));
  }

  std::weak_ptr<io::IoIf> io_ctx{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    io_ctx = io_ctx_;
  }

  if (!temp_instance->start(io_ctx)) {
    throw EXCEPT(fmt::format("Failed to start eBPF service: {}", real_so_path));
  }

  auto so_path_key = std::string{so_path};
  std::string service_name{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (services_.find(so_path_key) != services_.end()) {
      LOG(warn, "Service already loaded: {}", so_path_key);
      return false;
    }

    service_name = temp_instance->getName();
    ServiceHandle serv{std::move(handle),
                       std::move(temp_instance),
                       service_name,
                       so_path_key,
                       {},
                       hook_type,
                       ifindex};

    services_[so_path_key] = std::move(serv);
  }

  if (real_so_path == ebpf::SCHEDULER_XDP_LIB || real_so_path == ebpf::SCHEDULER_TC_LIB) {
    ASSERT(services_[so_path_key].instance != nullptr);
    services_[so_path_key].instance->onConfigUpdate(hook_type, ifindex, {});
  } else {
    registerServiceConfig(real_so_path, hook_type, ifindex, config_path);
  }

  LOG(info, "Service \"{}\" loaded from {}", service_name, real_so_path);

  std::shared_ptr<ebpf::PinnedProgMap> chain_mgr{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    chain_mgr = chain_mgr_.lock();
    if (chain_mgr == nullptr) {
      LOG(warn, "Chain manager is not set");
      return false;
    }
  }

  ASSERT(chain_mgr != nullptr);
  auto *service = getService(so_path_key);
  if (service != nullptr) {
    auto cononical_name = ebpf::PinnedProgMap::getCanonicalName(
        hook_type, ebpf::PinnedProgMap::UnofficialName::PROGRAM, service_name);
    int prog_fd = service->getMainProgFd();
    if (prog_fd >= 0) {
      chain_mgr->insertProg(hook_type, cononical_name, prog_fd, ifindex);
    }
    LOG(info, "Service \"{}\"({}) registered to chain manager", service_name, prog_fd);
  }

  return true;
}

/**
 * @brief 卸载 eBPF 程序
 *
 * @param so_path eBPF 程序动态库路径（必须是 so 动态库）
 * @return true 卸载成功
 * @return false 卸载失败
 */
bool Loader::unloadServices(std::string_view so_path) {
  ASSERT(inotify_mgr_.lock());

  decltype(services_.begin()) iter{};
  std::string path{}, config{};

  {
    std::lock_guard<std::mutex> lock{mutex_};
    iter = services_.find(std::string{so_path});

    if (iter == services_.end()) {
      LOG(error, "Cannot find the service {}", so_path);
      return false;
    }

    config = iter->second.config;
    path = iter->second.path;
  }

  if (!config.empty()) {
    unregisterServiceConfig(path, config);
  }

  {
    std::lock_guard<std::mutex> lock{mutex_};
    ServiceHandle &serv = iter->second;
    if (serv.instance != nullptr) {
      serv.instance->stop();
      serv.instance.reset();
    } else {
      LOG(error, "Service \"{}\" not started", so_path);
      return false;
    }
    if (serv.handle != nullptr) {
      serv.handle.reset();
    } else {
      LOG(error, "The symbol of service \"{}\" was undefined", so_path);
      return false;
    }
    services_.erase(iter);
  }

  LOG(info, "Service \"{}\" unloaded", so_path);
  return true;
}

/**
 * @brief 获取 eBPF 程序实例（eBPF 服务）
 *
 * @param so_path eBPF 程序动态库路径（必须是 so 动态库）
 * @return EbpfIf* 对象
 */
ebpf::EbpfIf *Loader::getService(std::string_view so_path) const {
  std::lock_guard<std::mutex> lock{mutex_};
  auto it = services_.find(std::string{so_path});
  if (it == services_.end()) {
    return nullptr;
  }
  return it->second.instance.get();
}

/**
 * @brief 获取全部已经加载的 eBPF 程序
 *
 * @return std::vector<std::string> 全部已经加载的 eBPF 程序动态库路径
 * （必须是 so 动态库）与网卡索引
 */
std::vector<std::string> Loader::getAllService() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::string> services{};
  for (const auto &[name, handle] : services_) {
    services.emplace_back(name);
  }
  return services;
}

/**
 * @brief 将 eBPF 配置注册到 Inotify
 *
 * @param so_path eBPF 动态库路径
 * @param hook_type hook 类型
 * @param ifindex 网卡索引
 * @param config_path eBPF 配置路径
 */
void Loader::registerServiceConfig(std::string_view so_path, HookType hook_type, int ifindex,
                                   std::string_view config_path) {
  ASSERT(inotify_mgr_.lock());
  ASSERT(hook_type != HookType::UNKNOWN);
  ASSERT(ifindex > 0);

  if (config_path.empty()) {
    return;
  }

  std::shared_ptr<inotify::InotifyManagerIf> inotify_mngr{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    inotify_mngr = inotify_mgr_.lock();
    if (!inotify_mngr) {
      throw EXCEPT("Inotify is invalid");
    }
  }

  if (!inotify_mngr->watchConfig(
          config_path, inotify::InotifyCb{
                           FUNCTION_LINE, [this, so_path, hook_type, ifindex](std::string_view name,
                                                                              uint32_t mask) {
                             GLOBAL_LOG(trace, "Config file {} changed, mask is {:#x}", name, mask);
                             loadServiceConfigs(so_path, hook_type, ifindex, name);
                           }})) {
    throw EXCEPT("Inotify add watch failed");
  }
  loadServiceConfigs(so_path, hook_type, ifindex, config_path); // 初始化

  {
    std::lock_guard<std::mutex> lock{mutex_};
    auto iter = services_.find(std::string{so_path});
    if (iter == services_.end()) {
      throw EXCEPT("Service not found, MUST BE load service first");
    }
    iter->second.config = std::string{config_path};
  }

  LOG(info, "Inotify watch {}", config_path);
}

/**
 * @brief 取消注册 eBPF 配置
 *
 * @param so_path eBPF 动态库路径
 * @param config_path eBPF 配置路径
 */
void Loader::unregisterServiceConfig(std::string_view so_path, std::string_view config_path) {
  ASSERT(inotify_mgr_.lock());

  if (config_path.empty()) {
    return;
  }

  std::shared_ptr<inotify::InotifyManagerIf> inotify_mngr{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    inotify_mngr = inotify_mgr_.lock();
    if (!inotify_mngr) {
      throw EXCEPT("Inotify is invalid");
    }
  }

  inotify_mngr->goodbyeConfig(config_path);

  {
    std::lock_guard<std::mutex> lock{mutex_};
    auto iter = services_.find(std::string{so_path});
    if (iter == services_.end()) {
      throw EXCEPT("Service not found, MUST BE load service first");
    }
    iter->second.config.clear();
  }

  LOG(info, "Inotify remove the watching: {}", config_path);
}

/**
 * @brief 设置程序链表
 *
 * @param mgr 管理器
 * @return true 设置成功
 * @return false 设置失败
 */
bool Loader::setProgChainManager(std::weak_ptr<ebpf::PinnedProgMap> mgr) {
  chain_mgr_ = mgr;
  return static_cast<bool>(chain_mgr_.lock() != nullptr);
}

/**
 * @brief 分配程序链空间
 *
 * @param hook_type 绑定类型
 * @param ifindex 网卡索引
 * @return true 分配成功
 * @return false 分配失败
 */
bool Loader::allocateChain(HookType hook_type, int ifindex) {
  std::shared_ptr<ebpf::PinnedProgMap> chain_mgr{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    chain_mgr = chain_mgr_.lock();
    if (!chain_mgr) {
      LOG(warn, "Chain manager is not set");
      return false;
    }
  }
  return chain_mgr->allocateRegion(hook_type, ifindex);
}

/**
 * @brief 更新指定 hook 类型的尾调用链顺序
 *
 * @param hook_type 类型
 * @param names_ifindex 程序名称与网卡索引对
 * @return true 成功
 * @return false 失败
 */
bool Loader::updateProgChain(HookType hook_type,
                             const ebpf::PinnedProgMap::NamesIfindexVec &names_ifindex) {
  std::shared_ptr<ebpf::PinnedProgMap> chain_mgr{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    chain_mgr = chain_mgr_.lock();
    if (!chain_mgr) {
      LOG(warn, "Chain manager is not set");
      return false;
    }
  }
  return chain_mgr->updateChain(hook_type, names_ifindex);
}

/**
 * @brief 更新调试接口配置
 *
 * @param out 配置文件
 */
nlohmann::json Loader::getDebugStatus(void) {
  nlohmann::json arr = nlohmann::json::array();

  {
    std::lock_guard<std::mutex> lock{mutex_};
    for (const auto &[so_path, handle] : services_) {
      nlohmann::json svc{};
      svc["so_path"] = so_path;
      svc["name"] = handle.name;
      svc["config_path"] = handle.config;
      svc["hook"] = enumName(handle.hook);
      svc["ifindex"] = handle.ifindex;
      if (handle.instance != nullptr) {
        svc["status"] = handle.instance->getDebugStatus();
      }
      arr.push_back(std::move(svc));
    }

    auto chain = chain_mgr_.lock();
    if (chain != nullptr) {
      arr.push_back(chain->getDebugStatus());
    }
  }

  return arr;
}

/**
 * @brief 设置 io 模块
 *
 * @param io_ctx io 对象
 * @return true 设置成功
 * @return false 设置失败
 */
bool Loader::setIoContext(std::weak_ptr<io::IoIf> io_ctx) {
  io_ctx_ = io_ctx;
  return static_cast<bool>(io_ctx_.lock() != nullptr);
}

/**
 * @brief 设置 inotify 管理器
 *
 * @param inotify_manager 管理器对象
 * @return true 设置成功
 * @return false 设置失败
 */
bool Loader::setInotifyManager(std::weak_ptr<inotify::InotifyManagerIf> inotify_manager) {
  inotify_mgr_ = inotify_manager;
  return static_cast<bool>(inotify_mgr_.lock());
}

/**
 * @brief 加载 eBPF 程序配置
 *
 * @param so_path 动态库路径
 * @param hook_type hook 类型
 * @param ifindex 网卡索引
 * @param conf_path 配置路径
 */
void Loader::loadServiceConfigs(std::string_view so_path, HookType hook_type, int ifindex,
                                std::string_view conf_path) {
  ASSERT(hook_type != HookType::UNKNOWN);
  ASSERT(ifindex > 0);

  auto *service = getService(so_path);
  if (service == nullptr) {
    unregisterServiceConfig(so_path, conf_path);
    return;
  }

  nlohmann::json json{};
  try {
    std::ifstream file{std::string{conf_path}};
    file >> json;
  } catch (const std::exception &e) {
    LOG(error, "Failed to parse config for {}: {}", so_path, e.what());
  }

  service->onConfigUpdate(hook_type, ifindex, json);
  LOG(info, "Config updated for service {}", so_path);
}

/**
 * @brief 清空资源
 *
 */
void Loader::clear() noexcept {
  decltype(services_) dup{};

  try {
    for (auto &serv : dup) {
      auto &handle = serv.second;
      if (handle.handle != nullptr) {
        handle.handle.reset();
      }
      if (handle.instance != nullptr) {
        handle.instance->stop();
        handle.instance.reset();
      }
      if (!handle.config.empty()) {
        unregisterServiceConfig(handle.path, handle.config);
      }
    }
  } catch (const except::Exception &exc) {
    GLOBAL_LOG(error, "Failed to clear services: {}\nStackframe: {}", exc.what(), exc.stackFrame());
  } catch (const std::exception &exc) {
    GLOBAL_LOG(error, "Failed to clear services: {}", exc.what());
  }
}

/**
 * @brief 根据官方程序名获取绑定类型
 *
 * @param canonical_name 官方程序名
 * @return HookType 绑定类型
 */
HookType Loader::getHookType(std::string_view canonical_name) const {
  std::shared_ptr<ebpf::PinnedProgMap> chain_mgr{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    chain_mgr = chain_mgr_.lock();
    if (!chain_mgr) {
      LOG(warn, "Chain manager is not set");
      return HookType::UNKNOWN;
    }
  }

  int id{};
  HookType result{HookType::UNKNOWN};
  if (!chain_mgr->lookupProgInfo(canonical_name, id, result)) {
    return HookType::UNKNOWN;
  }

  return result;
}

} // namespace daemon
} // namespace hebpf
