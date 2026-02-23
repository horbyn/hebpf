#pragma once

// clang-format off
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include "daemon_if.h"
#include "loader_if.h"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace daemon {

class Daemon : public DaemonIf, public log::Loggable<log::Id::daemon> {
public:
  explicit Daemon(std::unique_ptr<LoaderIf> loader);

  void run() override;
  void stop() override;
  void OnConfigChanged(const Configs &config) override;

private:
  void eventLoop() noexcept;

  std::unique_ptr<LoaderIf> loader_;
  std::atomic<bool> running_{false};
  std::thread monitor_thread_;
  std::vector<std::string> current_so_;
};

} // namespace daemon
} // namespace hebpf
