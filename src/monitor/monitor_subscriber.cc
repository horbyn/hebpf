// clang-format off
#include "monitor_subscriber.h"
#include "src/common/assert.h"
#include "src/common/exception.h"
#include "src/data/metrics.h"
#include "src/data/kernel_data.h"
#include "src/thread/thread.h"
// clang-format on

namespace hebpf {
namespace monitor {

MonitorSubscriber::MonitorSubscriber(std::unique_ptr<MonitorFactoryIf> factory)
    : bind_address_{}, factory_{std::move(factory)} {}

/**
 * @brief 更新配置
 *
 * @param config 配置对象
 */
void MonitorSubscriber::update(const daemon::Configs &config) {
  bool enabled = config.getPrometheusEnabled();
  auto last_enabled = (monitor_ != nullptr);
  if (!enabled && !last_enabled) {
    return;
  }

  std::string listen = config.getPrometheusListen();
  auto need_restart = (bind_address_ != listen);
  if (enabled && last_enabled && !need_restart) {
    return;
  }

  if (!enabled || (enabled && need_restart)) {
    stop();
  }

  if (enabled) {
    run(listen);
  }
}

/**
 * @brief 启动 prometheus 监控
 *
 * @param bind_address 监听地址
 */
void MonitorSubscriber::run(std::string_view bind_address) {
  if (running_.exchange(true)) {
    return;
  }

  ASSERT(!bind_address.empty());
  ASSERT(factory_ != nullptr);
  ASSERT(queue_ != nullptr);

  std::lock_guard<std::mutex> lock{mutex_};

  {
    bind_address_ = std::string{bind_address};

    monitor_ = factory_->create(bind_address_);

    consumer_thread_ = std::make_unique<thread::Thread>();
    consumer_thread_->setName(NAME_MONITOR_COUS);
    consumer_thread_->start(std::bind(&MonitorSubscriber::consumeLoop, this));

    queue_->start();
  }

  LOG(info, "Prometheus monitor is running on {}", bind_address_);
}

/**
 * @brief 停止 prometheus 监控
 *
 */
void MonitorSubscriber::stop() {
  if (!running_.exchange(false)) {
    return;
  }

  ASSERT(queue_ != nullptr);

  {
    std::lock_guard<std::mutex> lock{mutex_};
    queue_->stop();

    if (consumer_thread_ != nullptr && consumer_thread_->joinable()) {
      consumer_thread_->join();
      consumer_thread_.reset();
    }
    if (monitor_ != nullptr) {
      monitor_.reset();
    }
    bind_address_.clear();
  }

  LOG(info, "Prometheus monitor stopped");
}

/**
 * @brief 设置线程安全队列
 *
 * @param queue 队列
 */
void MonitorSubscriber::setQueue(std::shared_ptr<QueueDaemonMonitor> queue) { queue_ = queue; }

/**
 * @brief 消费者事件循环
 *
 */
void MonitorSubscriber::consumeLoop() noexcept {
  ASSERT(queue_ != nullptr);

  try {
    while (running_.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      auto status_opt = queue_->pop();
      if (!status_opt.has_value()) {
        continue;
      }

      Metrics metrics = status_opt.value();
      std::string service_name = metrics.service_;
      const auto &vec = metrics.metrics_;
      for (const auto &elem : vec) {
        using namespace services::examples;

        std::shared_ptr<MonitorIf> mon{};
        {
          std::lock_guard<std::mutex> lock{mutex_};
          mon = monitor_;
        }
        if (mon == nullptr) {
          continue;
        }

        KernelData event = elem;
        mon->incrementCounter(event.name_, event.labels_, event.value_);
      } // end for()

    } // end while()
  } catch (const except::Exception &exc) {
    GLOBAL_LOG(error, "MonitorSubscriber consumer exception: {}\n{}", exc.what(), exc.stackFrame());
  } catch (const std::exception &exc) {
    GLOBAL_LOG(error, "MonitorSubscriber consumer exception: {}", exc.what());
  }
}

} // namespace monitor
} // namespace hebpf
