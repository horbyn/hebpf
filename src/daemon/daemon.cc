// clang-format off
#include "daemon.h"
#include <pthread.h>
#include "loader.h"
#include "src/common/assert.h"
#include "src/common/exception.h"
#include "src/thread/thread.h"
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

  {
    std::lock_guard<std::mutex> lock{queue_mutex_};
    if (status_queue_ != nullptr) {
      producer_thread_ = std::make_unique<thread::Thread>();
      producer_thread_->setName(NAME_DAEMON_PROD);
      producer_thread_->start(std::bind(&Daemon::produceLoop, this));
    }
  }
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

  if (loader_ != nullptr) {
    const auto &all_ebpf = loader_->getAllService();
    for (const auto &so_name : all_ebpf) {
      auto *service = loader_->getService(so_name);
      if (service != nullptr) {
        service->stop();
      }
      loader_->unloadServices(so_name);
    }
  }
  LOG(info, "eBPF programs stopped");

  {
    std::lock_guard<std::mutex> lock{queue_mutex_};
    if (producer_thread_ != nullptr && producer_thread_->joinable()) {
      producer_thread_->join();
      producer_thread_.reset();
    }
    if (status_queue_ != nullptr) {
      status_queue_->stop();
      status_queue_.reset();
    }
  }
}

/**
 * @brief 更新配置文件
 *
 * @param config 配置文件对象
 */
void Daemon::update(const Configs &config) {
  ASSERT(loader_ != nullptr);
  auto new_so = config.getEbpf();

  std::lock_guard<std::mutex> lock{mutex_};

  // 需要卸载的（current 中存在但 new 中不存在）
  for (const auto &so : current_so_) {
    if (std::find(new_so.begin(), new_so.end(), so) == new_so.end()) {
      auto *service = loader_->getService(so);
      if (service == nullptr) {
        GLOBAL_LOG(error, "Cannot get service in unloading: ", so);
        continue;
      }

      service->stop();

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

      if (!service->start(io_ctx_)) {
        GLOBAL_LOG(error, "Failed to start BPF service");
        continue;
      }
    }
  }
  current_so_ = new_so;
}

/**
 * @brief 设置 io 模块
 *
 * @param io_ctx io 对象
 * @return true 设置成功
 * @return false 设置失败
 */
bool Daemon::setIoContext(std::weak_ptr<io::IoIf> io_ctx) {
  io_ctx_ = io_ctx;
  return static_cast<bool>(io_ctx_.lock() != nullptr);
}

/**
 * @brief 设置单生产者-单消费者队列
 *
 * @param queue
 */
void Daemon::setStatusQueue(std::shared_ptr<QueueDaemonMonitor> queue) { status_queue_ = queue; }

/**
 * @brief Daemon 生产者事件循环
 *
 */
void Daemon::produceLoop() noexcept {
  constexpr int sleep_time_sec = 1;

  try {
    while (running_.load()) {
      std::vector<std::string> services{};

      {
        std::lock_guard<std::mutex> lock{mutex_};
        services = current_so_;
      }

      for (const auto &so_path : services) {
        const auto *service = loader_->getService(so_path);
        if (service != nullptr) {
          auto status = service->getStatus();
          if (!status.empty()) {
            std::shared_ptr<QueueDaemonMonitor> queue{};
            {
              std::lock_guard<std::mutex> lock{queue_mutex_};
              queue = status_queue_;
            }
            if (queue != nullptr && !queue->full()) {
              queue->push(status);
            }
          }
        }
      } // end for()

      std::this_thread::sleep_for(std::chrono::seconds(sleep_time_sec));
    } // end while()
  } catch (const except::Exception &exc) {
    LOG(error, "Daemon productor dropped: {}\nDaemon stackframe:\n{}", exc.what(),
        exc.stackFrame());
  } catch (const std::exception &exc) {
    LOG(error, "Daemon productor dropped: {}", exc.what());
  }
}

} // namespace daemon
} // namespace hebpf
