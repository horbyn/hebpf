#pragma once

namespace hebpf {
namespace signal {

class SignalIf {
public:
  using SignalCb = void (*)(int sig);

  virtual ~SignalIf() = default;
  virtual void registerSignal(int sig, SignalCb cb) = 0;
  virtual bool shouldStop() const noexcept = 0;
  // 因为信号通常是全局唯一的，即多个模块共用同一个信号对象
  // 所以每个模块在接收到信号来清理资源时，应该复制信号对象背后的 fd
  // 避免某一个模块关闭了 fd 导致影响到所有模块
  virtual int dupStopFd() const noexcept = 0;
  virtual void triggerStop() = 0;
};

} // namespace signal
} // namespace hebpf
