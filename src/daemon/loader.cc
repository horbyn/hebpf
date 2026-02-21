// clang-format off
#include "loader.h"
#include "src/common/assert.h"
#include "src/common/exception.h"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace daemon {

/**
 * @brief 加载 eBPF 程序
 *
 * @param so_path eBPF 程序动态库路径（必须是 so 动态库）
 * @return std::string eBPF 程序名称
 */
std::string Loader::loadService(std::string_view so_path) {
  ASSERT(!so_path.empty());
  std::lock_guard<std::mutex> lock(mutex_);

  std::unique_ptr<void, DlCloser> handle(dlopen(so_path.data(), RTLD_LAZY | RTLD_LOCAL));
  if (handle == nullptr) {
    throw EXCEPT(fmt::format("dlopen() failed: {}", dlerror()));
  }

  auto create_fn = reinterpret_cast<CreateServiceFn>(dlsym(handle.get(), SERVICE_CREATE.data()));
  if (create_fn == nullptr) {
    throw EXCEPT(fmt::format("Failed to get factory symbols from {}", so_path));
  }

  auto temp_instance = create_fn();
  if (temp_instance == nullptr) {
    throw EXCEPT(fmt::format("Exported function from {} returned nullptr", so_path));
  }
  std::string service_name = temp_instance->getName();

  if (services_.find(service_name) != services_.end()) {
    LOG(warn, "Service already loaded: {}", service_name);
    return service_name;
  }

  ServiceHandle serv{std::move(handle), std::move(temp_instance), service_name,
                     std::string{so_path}};

  services_[service_name] = std::move(serv);
  LOG(info, "Service \"{}\" loaded from {}", service_name, so_path);
  return service_name;
}

/**
 * @brief 卸载 eBPF 程序
 *
 * @param service_name eBPF 程序名称
 * @return true 卸载成功
 * @return false 卸载失败
 */
bool Loader::unloadServices(std::string_view service_name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = services_.find(std::string{service_name});
  if (it == services_.end()) {
    return false;
  }

  ServiceHandle &serv = it->second;
  if (serv.instance != nullptr) {
    serv.instance.reset();
  }
  if (serv.handle != nullptr) {
    serv.handle.reset();
  }
  services_.erase(it);

  LOG(info, "Service \"{}\" unloaded", service_name);
  return true;
}

/**
 * @brief 获取 eBPF 程序实例（eBPF 服务）
 *
 * @param service_name eBPF 程序名称
 * @return EbpfIf* 对象
 */
EbpfIf *Loader::getService(std::string_view service_name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = services_.find(std::string{service_name});
  if (it == services_.end()) {
    return nullptr;
  }
  return it->second.instance.get();
}

/**
 * @brief 获取全部已经加载的 eBPF 程序
 *
 * @return std::vector<std::string> 全部已经加载的 eBPF 程序
 */
std::vector<std::string> Loader::getAllService() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<std::string> services{};
  for (const auto &[name, handle] : services_) {
    services.push_back(name);
  }
  return services;
}

} // namespace daemon
} // namespace hebpf
