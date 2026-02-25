#pragma once

// clang-format off
#include <atomic>
#include <memory>
#include "signal_if.h"
#include "src/log/logger.h"
#include "src/fd/fd_if.h"
// clang-format on

namespace hebpf {
namespace signal {

class Signal : public SignalIf, public log::Loggable<log::Id::signal> {
public:
  static Signal &instance();

  Signal(const Signal &) = delete;
  Signal &operator=(const Signal &) = delete;

  void registerSignal(int sig, SignalCb cb) override;
  bool shouldStop() const noexcept;
  int dupStopFd() const noexcept;
  void triggerStop();

  static void commonSignalHandler(int sig);
  static void crashSignalHandler(int sig);

private:
  explicit Signal(std::unique_ptr<FdIf> stop_fd = nullptr);
  ~Signal() = default;

  std::unique_ptr<FdIf> stop_fd_;
  std::atomic<bool> stop_;
};

} // namespace signal
} // namespace hebpf
