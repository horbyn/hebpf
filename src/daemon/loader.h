#pragma once

// clang-format off
#include <dlfcn.h>
#include <mutex>
#include <unordered_map>
#include "src/daemon/loader_if.h"
#include "src/inotify/inotify_manager_if.h"
#include "src/io/io_if.h"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace daemon {

/**
 * @brief eBPF 程序删除器
 *
 */
struct DlCloser {
  void operator()(void *h) const noexcept {
    if (h)
      dlclose(h);
  }
};

/**
 * @brief eBPF 程序实例
 *
 */
struct ServiceHandle {
  std::unique_ptr<void, DlCloser> handle; // 动态库 handle
  std::unique_ptr<ebpf::EbpfIf> instance; // eBPF 对象
  std::string name;                       // eBPF 程序名称
  std::string path;                       // eBPF 动态库路径
  std::string config;                     // eBPF 配置路径
};

class Loader : public LoaderIf, public log::Loggable<log::Id::daemon> {
public:
  ~Loader();

  bool loadService(std::string_view so_path, std::string_view config_path) override;
  bool unloadServices(std::string_view so_path) override;
  ebpf::EbpfIf *getService(std::string_view so_path) const override;
  std::vector<std::string> getAllService() const override;
  void registerServiceConfig(std::string_view so_path, std::string_view config_path) override;
  void unregisterServiceConfig(std::string_view so_path, std::string_view config_path) override;

  bool setIoContext(std::weak_ptr<io::IoIf> io_ctx);
  bool setInotifyManager(std::weak_ptr<inotify::InotifyManagerIf> inotify_manager);

private:
  void loadServiceConfigs(std::string_view so_path, std::string_view conf_path);
  void clear() noexcept;

  std::unordered_map<std::string, ServiceHandle> services_;
  mutable std::mutex mutex_;
  std::weak_ptr<io::IoIf> io_ctx_;
  std::weak_ptr<inotify::InotifyManagerIf> inotify_mgr_;
};

} // namespace daemon
} // namespace hebpf
