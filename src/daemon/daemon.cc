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
      unloadEbpf(so_name);
    }
    current_ebpf_.clear();
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
  auto new_ebpf = config.getEbpfs();

  decltype(new_ebpf) current_ebpf_dup{};
  daemon::LoaderIf *loader{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    current_ebpf_dup = current_ebpf_;
    loader = loader_.get();
  }

  // 需要卸载的
  for (const auto &elem : current_ebpf_dup) {
    if (new_ebpf.find(elem.first) == new_ebpf.end()) {
      auto lib = elem.second.getLib();
      unloadEbpf(lib);
    }
  }

  // 需要加载的
  for (const auto &elem : new_ebpf) {
    auto new_lib = elem.second.getLib();
    auto new_conf = elem.second.getConfig();

    auto current_it = current_ebpf_dup.find(elem.first);
    if (current_it == current_ebpf_dup.end()) {
      // 加载
      loadEbpf(new_lib, new_conf);
    } else {
      // 更新
      auto current_lib = current_it->second.getLib();
      if (current_lib != new_lib) {
        unloadEbpf(current_lib);
        loadEbpf(new_lib, new_conf);
        continue; // 更新动态库同时也会更新配置文件，所以后续不需再继续检查配置文件了
      }

      auto current_conf = current_it->second.getConfig();
      if (current_conf != new_conf) {
        loader->unregisterServiceConfig(current_lib, current_conf);
        loader->registerServiceConfig(new_lib, new_conf);
      }
    }
  }

  {
    std::lock_guard<std::mutex> lock{mutex_};
    current_ebpf_ = new_ebpf;
  }
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
      Configs::EbpfMap services{};

      {
        std::lock_guard<std::mutex> lock{mutex_};
        services = current_ebpf_;
      }

      for (const auto &pair : services) {
        auto lib = pair.second.getLib();
        const auto *service = loader_->getService(lib);
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

/**
 * @brief 加载 eBPF 程序
 *
 * @param so_path eBPF 动态库路径
 * @param config_path eBPF 配置路径
 */
void Daemon::loadEbpf(std::string_view so_path, std::string_view config_path) {
  ASSERT(!so_path.empty());
  ASSERT(loader_ != nullptr);

  std::weak_ptr<io::IoIf> io_ctx{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!loader_->loadService(so_path, config_path)) {
      return;
    }
  }
}

/**
 * @brief 卸载 eBPF 程序
 *
 * @param so_path eBPF 动态库路径
 */
void Daemon::unloadEbpf(std::string_view so_path) {
  ASSERT(!so_path.empty());
  ASSERT(loader_ != nullptr);

  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (!loader_->unloadServices(so_path)) {
      GLOBAL_LOG(error, "Cannot unload service: {}", so_path);
    }
  }
}

} // namespace daemon
} // namespace hebpf
