#pragma once

// clang-format off
#include <dlfcn.h>
#include <mutex>
#include <unordered_map>
#include "src/daemon/loader_if.h"
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
  std::unique_ptr<EbpfIf> instance;       // eBPF 对象
  std::string name;                       // eBPF 程序名称
  std::string path;                       // eBPF 动态库路径
};

class Loader : public LoaderIf, public log::Loggable<log::Id::daemon> {
public:
  bool loadService(std::string_view so_path) override;
  bool unloadServices(std::string_view so_path) override;
  EbpfIf *getService(std::string_view so_path) const override;
  std::vector<std::string> getAllService() const override;

private:
  std::unordered_map<std::string, ServiceHandle> services_;
  mutable std::mutex mutex_;
};

} // namespace daemon
} // namespace hebpf
