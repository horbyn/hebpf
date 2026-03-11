#pragma once

// clang-format off
#include <atomic>
#include <memory>
#include <mutex>
#include "src/data/queue_if.h"
#include "src/log/logger.h"
#include "src/thread/thread_if.h"
#include "monitor_subscriber_if.h"
#include "monitor_if.h"
#include "monitor_factory_if.h"
// clang-format on

namespace hebpf {
namespace monitor {

constexpr std::string_view NAME_MONITOR_COUS{"monitor-cous"};

class MonitorSubscriber : public MonitorSubscriberIf, public log::Loggable<log::Id::monitor> {
public:
  explicit MonitorSubscriber(std::string_view bind_address,
                             std::shared_ptr<MonitorFactoryIf> factory);
  void update(const daemon::Configs &config) override;

  void run();
  void stop();
  void setQueue(std::shared_ptr<QueueDaemonMonitor> queue);

private:
  void consumeLoop() noexcept;

  std::atomic<bool> running_{false};
  std::mutex mutex_;
  std::string bind_address_;
  std::shared_ptr<MonitorIf> monitor_;
  std::shared_ptr<QueueDaemonMonitor> queue_;
  std::unique_ptr<thread::ThreadIf> consumer_thread_;
  std::shared_ptr<MonitorFactoryIf> factory_;
};

} // namespace monitor
} // namespace hebpf
