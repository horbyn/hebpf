#pragma once

// clang-format off
#include "src/services/scheduler/scheduler_xdp.skel.h"
#include "scheduler_base.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

constexpr std::string_view SERVICE_NAME_SCHEDULER_XDP{"scheduler_xdp"};

class SchedulerXdp : public SchedulerBase<struct scheduler_xdp_bpf> {
public:
  explicit SchedulerXdp(std::unique_ptr<scheduler_xdp_bpf> skel = {});

  std::string getName() const override;
};

} // namespace scheduler
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
