#pragma once

// clang-format off
#include "src/ebpf/ebpf_with_net_hook.h"
#include "src/services/scheduler/scheduler_xdp.skel.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

class SchedulerXdp : public ebpf::EbpfWithNetHook<struct scheduler_xdp_bpf> {
public:
  explicit SchedulerXdp(std::unique_ptr<scheduler_xdp_bpf> skel = {});

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
