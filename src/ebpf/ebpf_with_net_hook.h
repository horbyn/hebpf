#pragma once

// clang-format off
#include <atomic>
#include <memory>
#include "ebpf_if.h"
#include "ebpf_net_hook_if.h"
#include "src/daemon/configs.hpp"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace ebpf {

/**
 * @brief 带网络 hook 的 eBPF 程序
 */
template <typename T>
class EbpfWithNetHook : public EbpfSkelIf<T>, public log::Loggable<log::Id::ebpf> {
public:
  EbpfWithNetHook(std::unique_ptr<T> skel = {});

  void detach() override;

protected:
  // 引入依赖基类的日志相关函数，使 LOG 宏可用

  using log::Loggable<log::Id::ebpf>::getLevel;
  using log::Loggable<log::Id::ebpf>::getModuleLogger;

  bool attachGeneric(daemon::HookType hook_type, int ifindex);

  std::unique_ptr<EbpfNetHookIf> hook_;
  int ifindex_;
  std::atomic<bool> attached_;
};

} // namespace ebpf
} // namespace hebpf

#include "ebpf_with_net_hook.tpp"
