#pragma once

// clang-format off
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include "daemon_if.h"
#include "loader_if.h"
#include "src/log/logger.h"
#include "src/io/io_if.h"
#include "src/thread/thread_if.h"
// clang-format on

namespace hebpf {
namespace daemon {

constexpr std::string_view NAME_DAEMON{"daemon"};

class Daemon : public DaemonIf, public log::Loggable<log::Id::daemon> {
public:
  explicit Daemon(std::unique_ptr<LoaderIf> loader);

  void run() override;
  void stop() override;
  void OnConfigChanged(const Configs &config) override;

  bool setIoContext(std::weak_ptr<io::IoIf> io_ctx);

private:
  void eventLoop() noexcept;

  std::unique_ptr<LoaderIf> loader_;
  std::atomic<bool> running_{false};
  std::unique_ptr<thread::ThreadIf> monitor_thread_;
  std::vector<std::string> current_so_;
  std::weak_ptr<io::IoIf> io_ctx_;
};

} // namespace daemon
} // namespace hebpf
