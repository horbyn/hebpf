// clang-format off
#include "daemon.h"
#include <pthread.h>
#include <algorithm>
#include <unordered_set>
#include "src/common/assert.h"
#include "src/common/exception.h"
#include "src/data/metrics.h"
#include "src/thread/thread.h"
#include "daemon_json.h"
// clang-format on

namespace hebpf {
namespace daemon {

Daemon::Daemon(std::unique_ptr<ConfigManager> conf_mngr, std::unique_ptr<ebpf::Region> region,
               std::unique_ptr<ebpf::EbpfManager> ebpf_mngr,
               std::unique_ptr<ebpf::PinnedMapManager> pinned_mngr)
    : running_{false}, conf_mngr_{std::move(conf_mngr)}, region_{std::move(region)},
      ebpf_mngr_{std::move(ebpf_mngr)}, pinned_mngr_{std::move(pinned_mngr)} {}

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
      LOG(info, "Daemon productor started");
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

  {
    std::lock_guard<std::mutex> lock{queue_mutex_};
    if (producer_thread_ != nullptr && producer_thread_->joinable()) {
      producer_thread_->join();
      producer_thread_.reset();

      LOG(info, "Daemon productor stopped");
    }
    if (status_queue_ != nullptr) {
      status_queue_->stop();
      status_queue_.reset();
    }
  }
}

/**
 * @brief 更新调试接口配置
 *
 * @param out 配置文件
 */
void Daemon::update(nlohmann::json &out) {
  DaemonJson obj{};
  obj.setRunning(running_.load());

  {
    std::lock_guard<std::mutex> lock{queue_mutex_};
    if (status_queue_ != nullptr) {
      obj.setQueueEmpty(status_queue_->empty());
      obj.setQueueFull(status_queue_->full());
    }

    if (conf_mngr_ != nullptr) {
      obj.setConfigurations(conf_mngr_->getDebugStatus());
    }
    if (region_ != nullptr) {
      obj.setRegions(region_->getDebugStatus());
    }
    if (ebpf_mngr_ != nullptr) {
      obj.setServices(ebpf_mngr_->getDebugStatus());
    }
    if (pinned_mngr_ != nullptr) {
      obj.setPinnedMaps(pinned_mngr_->getDebugStatus());
    }
  }

  out[JKEY_DBGSERVER_DAEMON] = obj;
}

/**
 * @brief 更新配置文件
 *
 * @param config 配置文件对象
 */
void Daemon::update(const Configs &config) {
  ASSERT(ebpf_mngr_ != nullptr);
  ASSERT(conf_mngr_ != nullptr);
  ASSERT(pinned_mngr_ != nullptr);

  conf_mngr_->update(config.getEbpfs());
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
      if (ebpf_mngr_ != nullptr) {
        auto services = ebpf_mngr_->getAllServices();
        for (const auto service : services) {
          if (service != nullptr) {
            auto status = service->getPrometheusStatus();
            if (!status.contains(JKEY_METRICS_METRICS) || status.at(JKEY_METRICS_METRICS).empty()) {
              continue;
            }
            std::shared_ptr<QueueDaemonMonitor> queue{};
            {
              std::lock_guard<std::mutex> lock{queue_mutex_};
              queue = status_queue_;
            }
            if (queue != nullptr && !queue->full()) {
              queue->push(status);
            }
          } // end if(service != nullptr)
        } // end for()
      } // end if(ebpf_mngr_ != nullptr)

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
