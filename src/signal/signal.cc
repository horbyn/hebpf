// clang-format off
#include "signal.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <csignal>
#include "src/common/assert.h"
#include "src/common/stack_frame.h"
#include "src/fd/fd.h"
// clang-format on

namespace hebpf {
namespace signal {

/**
 * @brief 获取信号单例
 *
 * @return Signal& 单例引用
 */
Signal &Signal::instance() {
  static Signal inst{};
  return inst;
}

/**
 * @brief 注册信号
 *
 * @param sig 信号
 * @param cb 信号回调函数
 */
void Signal::registerSignal(int sig, SignalCb cb) { std::signal(sig, cb); }

/**
 * @brief 是否需要停止
 *
 * @return true 需要
 * @return false 不需要
 */
bool Signal::shouldStop() const noexcept { return stop_.load(); }

/**
 * @brief 拷贝停止文件描述符
 *
 * @return int 文件描述符
 */
int Signal::dupStopFd() const noexcept {
  ASSERT(stop_fd_ != nullptr);
  return dup(stop_fd_->fd());
}

/**
 * @brief 手动触发停止
 *
 */
void Signal::triggerStop() {
  if (stop_.load()) {
    return;
  }

  ASSERT(stop_fd_ != nullptr);
  ASSERT(stop_fd_->fd() != FD_INVALID);
  stop_.store(true);

  uint64_t val = 1;
  write(stop_fd_->fd(), &val, sizeof(val));
}

/**
 * @brief 普通信号处理函数
 *
 * @param sig 任意信号
 */
void Signal::commonSignalHandler(int sig) {
  const char *sig_str = nullptr;
  switch (sig) {
  case SIGINT:
    sig_str = "SIGINT";
    break;
  case SIGTERM:
    sig_str = "SIGTERM";
    break;
  case SIGSEGV:
    sig_str = "SIGSEGV";
    break;
  default:
    sig_str = "UNKNOWN";
    break;
  }

  const char *prefix = "\tCaught signal: ";
  ::write(STDERR_FILENO, prefix, strlen(prefix));
  ::write(STDERR_FILENO, sig_str, strlen(sig_str));
  ::write(STDERR_FILENO, "\n", 1);
  auto &inst = Signal::instance();
  inst.triggerStop();
}

/**
 * @brief 段错误信号处理函数
 *
 * @param sig 段错误处理信号
 */
void Signal::crashSignalHandler(int sig) {
  Signal::commonSignalHandler(sig);
  hebpf::stackTrace(true);

  // 恢复默认信号
  std::signal(sig, SIG_DFL);
  std::raise(sig);
}

Signal::Signal(std::unique_ptr<FdIf> stop_fd) {
  stop_fd_ = std::move(stop_fd);
  if (stop_fd_ == nullptr) {
    stop_fd_ = std::make_unique<Fd>(eventfd(0, EFD_NONBLOCK));
  }
}

} // namespace signal
} // namespace hebpf
