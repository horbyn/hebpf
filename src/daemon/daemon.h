#pragma once

// clang-format off
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "daemon_if.h"
#include "src/data/queue_if.h"
#include "src/ebpf/ebpf_manager.h"
#include "src/ebpf/pinned_map_manager.h"
#include "src/ebpf/region.h"
#include "src/log/logger.h"
#include "src/monitor/monitor_if.h"
#include "src/subscribe/json_subscriber_if.h"
#include "src/subscribe/yaml_subscriber_if.h"
#include "src/thread/thread_if.h"
#include "config_manager.h"
// clang-format on

namespace hebpf {
namespace daemon {

constexpr std::string_view NAME_DAEMON_PROD{"daemon-prod"};
constexpr std::string_view JKEY_DBGSERVER_DAEMON{"Daemon"};

class Daemon : public DaemonIf,
               public subscribe::JsonSubscriberIf,
               public subscribe::YamlSubscriberIf,
               public log::Loggable<log::Id::daemon> {
public:
  explicit Daemon(std::unique_ptr<ConfigManager> conf_mngr, std::unique_ptr<ebpf::Region> region,
                  std::unique_ptr<ebpf::EbpfManager> ebpf_mngr,
                  std::unique_ptr<ebpf::PinnedMapManager> pinned_mngr);

  void run() override;
  void stop() override;
  void update(nlohmann::json &out) override;
  void update(const Configs &config) override;

  void setStatusQueue(std::shared_ptr<QueueDaemonMonitor> queue);

private:
  void produceLoop() noexcept;

  std::atomic<bool> running_;
  std::unique_ptr<ConfigManager> conf_mngr_;            // 对象内部已考虑线程安全, 不需要外部加锁
  std::unique_ptr<ebpf::Region> region_;                // 对象内部已考虑线程安全, 不需要外部加锁
  std::unique_ptr<ebpf::EbpfManager> ebpf_mngr_;        // 对象内部已考虑线程安全, 不需要外部加锁
  std::unique_ptr<ebpf::PinnedMapManager> pinned_mngr_; // 直接操作 eBPF map

  std::mutex mutex_;
  std::shared_ptr<QueueDaemonMonitor> status_queue_;
  std::mutex queue_mutex_;
  std::unique_ptr<thread::ThreadIf> producer_thread_;
};

} // namespace daemon
} // namespace hebpf
