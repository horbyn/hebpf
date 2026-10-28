#pragma once

// clang-format off
#include "src/ebpf/ebpf_if.h"
#include "src/services/scheduler/scheduler_sched_switch.skel.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

class SchedulerSchedSwitch : public ebpf::EbpfSkelIf<struct scheduler_sched_switch_bpf> {
public:
  explicit SchedulerSchedSwitch(std::unique_ptr<scheduler_sched_switch_bpf> skel = {});

  std::string getName() const override;
  int getRunningFd(daemon::HookType hook_type) const override;
};

} // namespace scheduler
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
