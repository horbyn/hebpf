#pragma once

// clang-format off
#include "src/ebpf/ebpf_with_net_hook.h"
#include "src/services/scheduler/scheduler_tc.skel.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

class SchedulerTc : public ebpf::EbpfWithNetHook<struct scheduler_tc_bpf> {
public:
  explicit SchedulerTc(std::unique_ptr<scheduler_tc_bpf> skel = {});

  std::string getName() const override;
  int getRunningFd(daemon::HookType hook_type) const override;
  [[nodiscard]] bool onConfigUpdate(const nlohmann::json &config) override;
};

} // namespace scheduler
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
