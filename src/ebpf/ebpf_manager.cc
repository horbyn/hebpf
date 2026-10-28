// clang-format off
#include "ebpf_manager.h"
#include <fstream>
#include <unordered_set>
#include "src/common/exception.h"
#include "nlohmann/json.hpp"
#include "src/common/common.h"
// clang-format on

namespace hebpf {
namespace ebpf {

EbpfManager::EbpfManager(std::weak_ptr<event::EventBusIf> event_bus)
    : event::EventBusHolder(event_bus), services_{std::make_shared<ServiceMap>()} {
  auto bus = bus_.lock();
  if (bus == nullptr) {
    throw EXCEPT("event bus is null");
  }
  bus->subscribe<event::HashEvent>(
      event::EventHandler{FUNCTION_LINE, [this](const event::EventIf &eve) { onHashEvent(eve); }});
  bus->subscribe<event::ProgArrayFailureEvent>(event::EventHandler{
      FUNCTION_LINE, [this](const event::EventIf &eve) { onProgArrayFailureEvent(eve); }});
}

EbpfManager::~EbpfManager() {
  if (services_ == nullptr) {
    return;
  }

  // 因为 unloadService() 内部会从 services_ 中删除迭代器
  // 所以这里先拷贝一份，避免一边遍历 services_ 一边删除迭代器
  std::vector<ServiceKey> keys;
  keys.reserve(services_->size());
  for (const auto &kv : *services_) {
    keys.push_back(kv.first);
  }

  for (const auto &key : keys) {
    unloadService(key);
  }
}

/**
 * @brief 获取所有已加载的 eBPF 程序
 *
 * @return std::vector<EbpfIf *> 程序数组
 */
std::vector<EbpfIf *> EbpfManager::getAllServices() const {
  std::vector<EbpfIf *> result{};
  if (services_ == nullptr) {
    return result;
  }

  {
    std::lock_guard<std::mutex> lock{mutex_};
    for (auto &elem : *services_) {
      result.push_back(elem.second.instance.get());
    }
  }
  return result;
}

/**
 * @brief 更新调试接口配置
 *
 * @param out 配置文件
 */
nlohmann::json EbpfManager::getDebugStatus() const {
  if (services_ == nullptr) {
    return nlohmann::json{};
  }
  return *services_;
}

/**
 * @brief 设置 io 模块
 *
 * @param io_ctx io 对象
 * @return true 设置成功
 * @return false 设置失败
 */
bool EbpfManager::setIoContext(std::weak_ptr<io::IoIf> io_ctx) {
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
bool EbpfManager::setInotifyManager(std::weak_ptr<inotify::InotifyManagerIf> inotify_manager) {
  inotify_mgr_ = inotify_manager;
  return static_cast<bool>(inotify_mgr_.lock());
}

/**
 * @brief 增量加载卸载 eBPF 程序
 *
 * @param diff 增量配置（返回值）
 * @param result 结果集（返回值）
 * @param rollback 是否回退（true 回退，false 非回退）
 *
 * @return true 应用成功；false 应用失败
 */
bool EbpfManager::applyDiff(std::vector<daemon::Diff> &diff, daemon::HookMap &result,
                            bool rollback) {
  auto failed = false;
  std::unordered_set<daemon::HookType> load_filters{};

  for (auto iter = diff.begin(); iter != diff.end();) {
    auto elem = *iter;
    auto name = elem.getConfig()->getName();
    auto hook_key = elem.getHook();
    int running_fd = FD_INVALID;
    ServiceKey serv_key{hook_key, name};

    {
      std::lock_guard<std::mutex> lock{mutex_};
      switch (elem.getType()) {
      case daemon::DiffType::ADD:
        if (rollback) {
          // 回退需要反过来
          if (!unloadService(serv_key)) {
            LOG(warn, "Rollback failed to unload service: {}", name);
          }
        } else {
          if (load_filters.find(hook_key.hook) != load_filters.end()) {
            failed = true;
          } else {
            running_fd = loadService(serv_key, elem.getConfig());
            if (running_fd == FD_INVALID) {
              if (daemon::ConfigEbpf::isScheduler(name)) {
                load_filters.insert(hook_key.hook);
              }
              LOG(error, "Failed to load service: {}", name);
              failed = true;
            }
          }
        }
        break;
      case daemon::DiffType::REMOVE:
        if (rollback) {
          if (load_filters.find(hook_key.hook) != load_filters.end()) {
            failed = true;
          } else {
            // 回退需要反过来
            running_fd = loadService(serv_key, elem.getConfig());
            if (running_fd == FD_INVALID) {
              if (daemon::ConfigEbpf::isScheduler(name)) {
                load_filters.insert(hook_key.hook);
              }
              LOG(warn, "Rollback failed to load service: {}", name);
              failed = true;
            }
          }
        } else {
          if (!unloadService(serv_key)) {
            LOG(error, "Failed to unload service: {}", name);
            failed = true;
          }
        }
        break;
      default:
        break;
      } // end switch-case()
    } // end lock

    if (failed) {
      if (result.find(hook_key) != result.end()) {
        auto &result_prog_list = result[hook_key];
        auto find_iter = std::find_if(
            result_prog_list.begin(), result_prog_list.end(),
            [&name](const daemon::ConfigEbpfPtr &elem) { return elem->getName() == name; });
        if (find_iter != result_prog_list.end()) {
          result_prog_list.erase(find_iter);
        }
        if (result_prog_list.empty()) {
          result.erase(hook_key);
        }
      }
      iter = diff.erase(iter);
    } else {
      ++iter;
    } // end if-else(remove)

  } // end for(diff)

  if (!rollback) {
    diff_ = diff; // 拷贝一份进行缓存
  }

  return failed;
}

/**
 * @brief 获取 eBPF 程序实例（不加锁）
 *
 * @param hook 键
 * @return EbpfIf* 对象
 */
EbpfIf *EbpfManager::getService(const ServiceKey &hook) const {
  if (services_ == nullptr) {
    return nullptr;
  }

  auto it = services_->find(hook);
  if (it == services_->end()) {
    return nullptr;
  }
  return it->second.instance.get();
}

/**
 * @brief 加载 eBPF 程序（不加锁）
 *
 * @param config 配置对象
 * @return int 运行时 eBPF 程序 fd，出错返回 FD_INVALID
 */
int EbpfManager::loadService(const ServiceKey &hook, const daemon::ConfigEbpfPtr &config) {
  ASSERT(services_ != nullptr);
  ASSERT(hook.hook.hook != daemon::HookType::UNKNOWN);
  ASSERT(config != nullptr);

  int running_fd = FD_INVALID;
  try {
    auto so_path = config->getLib();
    std::unique_ptr<void, DlCloser> handle(dlopen(so_path.c_str(), RTLD_LAZY | RTLD_LOCAL));
    if (handle == nullptr) {
      LOG(error, "dlopen() failed: {}", dlerror());
      return FD_INVALID;
    }

    auto create_fn = reinterpret_cast<CreateServiceFn>(dlsym(handle.get(), SERVICE_CREATE.data()));
    if (create_fn == nullptr) {
      LOG(error, "Failed to get factory symbols from {}", so_path);
      return FD_INVALID;
    }

    auto temp_instance = create_fn();
    if (temp_instance == nullptr) {
      LOG(error, "Exported function from {} returned nullptr", so_path);
      return FD_INVALID;
    }

    if (!temp_instance->start(io_ctx_)) {
      LOG(error, "Failed to start eBPF service: {}", so_path);
      return FD_INVALID;
    }

    // TODO: 程序名称可以从动态库中获取，与配置名称区别开
    auto service_name = temp_instance->getName();
    running_fd = temp_instance->getRunningFd(hook.hook.hook);
    if (services_->find(hook) != services_->end()) {
      LOG(warn, "Service already loaded: {}", so_path);
      return running_fd;
    }

    (*services_)[hook] = ServiceHandle{std::move(handle), std::move(temp_instance), config};

    if (!registerServiceConfig(hook, config)) {
      LOG(warn, "Cannot load configuration of service \"{}\", but not impact the service running",
          service_name);
      return running_fd;
    }
    LOG(info, "Service \"{}\" loaded from {}", service_name, so_path);
  } catch (const except::Exception &exc) {
    LOG(error, "Failed to load eBPF service: {}", exc.what());
  }

  return running_fd;
}

/**
 * @brief 卸载 eBPF 程序（不加锁）
 *
 * @param hook 键
 * @return true 卸载成功
 * @return false 卸载失败
 */
bool EbpfManager::unloadService(const ServiceKey &hook) {
  ASSERT(services_ != nullptr);
  ASSERT(hook.hook.hook != daemon::HookType::UNKNOWN);

  try {
    auto iter = services_->find(hook);
    if (iter == services_->end()) {
      LOG(error, "Cannot find the configuration \"{}\"", iter->second.config->getName());
      return false;
    }

    const auto &config_ro = iter->second.config;
    if (!config_ro->getConfig().empty()) {
      // 配置存在时从 inotify 中取消注册
      unregisterServiceConfig(hook);
    }

    auto conf_name = config_ro->getName();
    auto &serv = iter->second;
    std::string serv_name{"UNKNOWN"};
    if (serv.instance != nullptr) {
      serv_name = serv.instance->getName();
      serv.instance->stop();
      serv.instance.reset();
    } else {
      LOG(error, "Configuration \"{}\" not started", conf_name);
      return false;
    }

    if (serv.handle != nullptr) {
      serv.handle.reset();
    } else {
      LOG(error, "The symbol of service \"{}\" was undefined", serv_name);
      return false;
    }
    services_->erase(iter);

    LOG(info, "Service \"{}\" unloaded", serv_name);
  } catch (const except::Exception &exc) {
    LOG(error, "Failed to unregister service \"{}\": {}", hook.name, exc.what());
    return false;
  }

  return true;
}

/**
 * @brief 将 eBPF 配置注册到 Inotify（不加锁）
 * @note 没有给出配置路径将什么都不做
 *
 * @param hook 键
 * @param config 配置对象
 * @return true 成功
 * @return false 失败
 */
bool EbpfManager::registerServiceConfig(const ServiceKey &hook,
                                        const daemon::ConfigEbpfPtr &config) {
  ASSERT(services_ != nullptr);
  ASSERT(hook.hook.hook != daemon::HookType::UNKNOWN);
  ASSERT(config != nullptr);

  auto name = config->getName();
  auto config_path = config->getConfig();
  if (config_path.empty()) {
    LOG(warn, "Configuration \"{}\" has no config path", name);
    return true;
  }

  auto inotify_mngr = inotify_mgr_.lock();
  if (inotify_mngr == nullptr) {
    LOG(error, "Inotify is invalid");
    return false;
  }

  if (!inotify_mngr->watchConfig(
          config_path, inotify::InotifyCb{
                           FUNCTION_LINE, [this, hook](std::string_view name, uint32_t mask) {
                             GLOBAL_LOG(trace, "Config file {} changed, mask is {:#x}", name, mask);
                             loadServiceConfig(hook);
                           }})) {
    LOG(error, "Inotify add watch failed");
    return false;
  }
  loadServiceConfig(hook); // 初始化

  auto iter = services_->find(hook);
  if (iter == services_->end()) {
    LOG(error, "Configuration \"{}\" not found, MUST BE load service first", name);
    return false;
  }
  iter->second.config->setConfig(config_path);

  LOG(info, "Inotify watch {}", config_path);
  return true;
}

/**
 * @brief 取消注册 eBPF 配置（不加锁）
 *
 * @param hook 键
 * @return true 成功
 * @return false 失败
 */
bool EbpfManager::unregisterServiceConfig(const ServiceKey &hook) {
  ASSERT(services_ != nullptr);
  ASSERT(hook.hook.hook != daemon::HookType::UNKNOWN);

  auto iter = services_->find(hook);
  if (iter == services_->end()) {
    LOG(error, "Configuration \"{}\" not found, MUST BE load service first",
        iter->second.config->getName());
    return false;
  }

  auto config_path = iter->second.config->getConfig();
  if (config_path.empty()) {
    return false;
  }

  auto inotify_mngr = inotify_mgr_.lock();
  if (inotify_mngr == nullptr) {
    LOG(error, "Inotify is invalid");
    return false;
  }

  if (!inotify_mngr->goodbyeConfig(config_path)) {
    return false;
  }

  auto serv_name = iter->second.instance->getName();
  iter->second.config->setConfig(""); // 删除配置路径

  LOG(info, "Inotify remove the watching \"{}\" for service \"{}\"", config_path, serv_name);
  return true;
}

/**
 * @brief 加载 eBPF 程序配置（不加锁）
 *
 * @param hook 键
 * @return true 成功
 * @return false 失败
 */
bool EbpfManager::loadServiceConfig(const ServiceKey &hook) {
  ASSERT(services_ != nullptr);
  ASSERT(hook.hook.hook != daemon::HookType::UNKNOWN);

  auto iter = services_->find(hook);
  if (iter == services_->end()) {
    LOG(error, "Service \"{}\" not found, MUST BE load service first",
        iter->second.config->getName());
    return false;
  }

  const auto &config_ro = iter->second.config;
  auto conf_path = config_ro->getConfig();
  auto *service = getService(hook);
  if (service == nullptr) {
    unregisterServiceConfig(hook);
    return true;
  }

  auto name = config_ro->getName();
  if (name.empty()) {
    throw EXCEPT("Bug: Could not get name of service");
  }

  nlohmann::json json{};
  if (daemon::ConfigEbpf::isScheduler(name)) {
    // scheduler 配置是内置的
    json = hook.hook;
  } else {
    // 其他程序是显式给出的
    try {
      std::ifstream file{conf_path};
      file >> json;
    } catch (const std::exception &e) {
      LOG(error, "Failed to parse config \"{}\" for service \"{}\": {}", conf_path, name, e.what());
    }
  }

  if (!service->onConfigUpdate(json)) {
    LOG(error, "Failed to update config \"{}\" for service \"{}\"", conf_path, name);
    return false;
  }
  LOG(info, "Config \"{}\" updated for service \"{}\"", conf_path, name);
  return true;
}

/**
 * @brief 内核态哈希表事件到达
 * @note Path: ConfigManager > Region > PinnedMap(hash) > EbpfManager
 *
 * @param event 事件对象
 */
void EbpfManager::onHashEvent(const event::EventIf &event) {
  auto map_ptr = event.getMap();
  auto &map = *map_ptr;
  auto diff_ptr = event.getDiff();
  auto &diff = *diff_ptr;
  auto ret = applyDiff(diff, map, false);

  std::shared_ptr<event::EventBusIf> bus{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    bus = bus_.lock();
    if (bus == nullptr) {
      throw EXCEPT("Event bus is invalid");
    }
  }

  if (ret) {
    applyDiff(diff, map, true);
    bus->publish(event::EbpfFailureEvent{map_ptr});
  }

  bus->publish(event::EbpfEvent{map_ptr, diff_ptr, services_});
}

/**
 * @brief 内核态程序数组失败事件到达
 * @note Path: ConfigManager > Region > PinnedMap(hash) > EbpfManager > PinnedMap(array)
 * @note                                                  EbpfManager < PinnedMap(array)（失败）
 *
 * @param event 事件对象
 */
void EbpfManager::onProgArrayFailureEvent(const event::EventIf &event) {
  auto map_ptr = event.getMap();
  auto &map = *map_ptr;
  auto diff_ptr = event.getDiff();
  auto &diff = *diff_ptr;

  // 形参传入的 diff 与缓存的 diff_ 是有差别的，差别的这部分才是需要回退的
  auto diff_bak = diff_;
  daemon::Diff::subtract(diff_bak, diff);
  applyDiff(diff_bak, map, true);
  diff_ = diff; // 更新缓存

  std::shared_ptr<event::EventBusIf> bus{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    bus = bus_.lock();
    if (bus == nullptr) {
      throw EXCEPT("Event bus is invalid");
    }
  }

  // 出错后返回上层，Hash 事件
  bus->publish(event::EbpfFailureEvent{map_ptr});
}

} // namespace ebpf
} // namespace hebpf
