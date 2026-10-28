#pragma once

// clang-format off
#include "src/ebpf/ebpf_with_net_hook.h"
#include "lb_rules.h"
// clang-format on

namespace hebpf {
namespace services {
namespace lb {

constexpr size_t MAX_RULES_SIZE{10240};

template <typename SkelType>
class LbBase : public ebpf::EbpfWithNetHook<SkelType> {
public:
  explicit LbBase(std::unique_ptr<SkelType> skel = {});

  int getRunningFd(daemon::HookType hook_type) const override;
  nlohmann::json getDebugStatus() const override;
  [[nodiscard]] bool onConfigUpdate(const nlohmann::json &config) override;

private:
  // 引入依赖基类的日志相关函数，使 LOG 宏可用

  using ebpf::EbpfWithNetHook<SkelType>::getLevel;
  using ebpf::EbpfWithNetHook<SkelType>::getModuleLogger;

protected:
  int getVipBackendsFd() const;
  int getBackendsPoolFd() const;
  int getLocalMacFd() const;
  void clearKernelRules();

  LbRules rules_;
};

} // namespace lb
} // namespace services
} // namespace hebpf

#include "lb_base.tpp"
