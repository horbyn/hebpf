#pragma once

// clang-format off
#include "src/ebpf/ebpf_if.h"
#include "src/services/scheduler/scheduler_block.skel.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

class SchedulerBlock : public ebpf::EbpfSkelIf<struct scheduler_block_bpf> {
public:
  explicit SchedulerBlock(std::unique_ptr<scheduler_block_bpf> skel = {});

  std::string getName() const override;
  void load() override;
  int getRunningFd(daemon::HookType hook_type) const override;
};

} // namespace scheduler
} // namespace services
} // namespace hebpf
