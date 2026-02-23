// clang-format off
#include "daemon.h"
#include "loader.h"
#include "src/common/assert.h"
#include "src/common/exception.h"
// clang-format on

namespace hebpf {
namespace daemon {

Daemon::Daemon(std::unique_ptr<LoaderIf> loader) : loader_{std::move(loader)} {}

/**
 * @brief 运行 deamon
 *
 */
void Daemon::run() {
  if (running_.load()) {
    return;
  }

  running_.store(true);
  monitor_thread_ = std::thread(&Daemon::eventLoop, this);
  LOG(info, "eBPF programs running! Press Ctrl+C to stop");
  LOG(info, "Run `sudo cat /sys/kernel/debug/tracing/trace_pipe` to view logs");
}

/**
 * @brief 停止 daemon
 *
 */
void Daemon::stop() {
  if (!running_.load()) {
    return;
  }

  running_.store(false);
  if (monitor_thread_.joinable()) {
    monitor_thread_.join();
  }

  const auto &all_ebpf = loader_->getAllService();
  for (const auto &so_name : all_ebpf) {
    auto *service = loader_->getService(so_name);
    service->detach();
    service->destroy();
  }
  LOG(info, "eBPF programs stopped");
}

/**
 * @brief 与配置文件 watcher 交互
 *
 * @param config 配置文件对象
 */
void Daemon::OnConfigChanged(const Configs &config) {
  ASSERT(loader_ != nullptr);
  auto new_so = config.getEbpf();

  // 需要卸载的（current 中存在但 new 中不存在）
  for (const auto &so : current_so_) {
    if (std::find(new_so.begin(), new_so.end(), so) == new_so.end()) {
      auto *service = loader_->getService(so);
      if (service == nullptr) {
        GLOBAL_LOG(error, "Cannot get service in unloading: ", so);
        continue;
      }
      service->detach();
      service->destroy();

      loader_->unloadServices(so);
    }
  }

  // 需要加载的（new 中存在但 current 中不存在）
  for (const auto &so : new_so) {
    if (std::find(current_so_.begin(), current_so_.end(), so) == current_so_.end()) {
      if (!loader_->loadService(so)) {
        continue;
      }

      auto *service = loader_->getService(so);
      if (service == nullptr) {
        GLOBAL_LOG(error, "Cannot get service in loading: ", so);
        continue;
      }

      // 打开 BPF skeleton
      service->open();
      if (!service) {
        GLOBAL_LOG(error, "Failed to open BPF service");
        continue;
      }

      // 加载并验证 eBPF 程序
      service->load();

      // 附加到挂载点
      service->attach();
    }
  }
  current_so_ = new_so;
}

/**
 * @brief Daemon 事件循环
 *
 */
void Daemon::eventLoop() noexcept {
  try {
    // TODO: 与 eBPF 程序交互，内核态负责提交它们的状态信息
  } catch (const except::Exception &exc) {
    LOG(error, "Daemon dropped: {}\nDaemon stackframe:\n{}", exc.what(), exc.stackFrame());
  } catch (const std::exception &exc) {
    LOG(error, "Daemon dropped: {}", exc.what());
  }
}

} // namespace daemon
} // namespace hebpf
