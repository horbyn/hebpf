#pragma once

// clang-format off
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "daemon_if.h"
#include "loader_if.h"
#include "src/log/logger.h"
#include "src/monitor/monitor_if.h"
#include "src/data/queue_if.h"
#include "src/subscribe/subscriber_if.h"
#include "src/thread/thread_if.h"
// clang-format on

namespace hebpf {
namespace daemon {

constexpr std::string_view NAME_DAEMON_PROD{"daemon-prod"};

class Daemon : public DaemonIf,
               public subscribe::SubscriberIf,
               public log::Loggable<log::Id::daemon> {
public:
  explicit Daemon(std::unique_ptr<LoaderIf> loader);

  void run() override;
  void stop() override;
  void update(const Configs &config) override;

  void setStatusQueue(std::shared_ptr<QueueDaemonMonitor> queue);

private:
  void produceLoop() noexcept;
  void loadEbpf(std::string_view so_path, std::string_view config_path);
  void unloadEbpf(std::string_view so_path);

  std::unique_ptr<LoaderIf> loader_;
  std::atomic<bool> running_{false};
  Configs::EbpfMap current_ebpf_;

  std::mutex mutex_;
  std::shared_ptr<QueueDaemonMonitor> status_queue_;
  std::mutex queue_mutex_;
  std::unique_ptr<thread::ThreadIf> producer_thread_;
};

} // namespace daemon
} // namespace hebpf
