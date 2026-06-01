#pragma once

// clang-format off
#include "src/services/scheduler/scheduler.skel.h"
#include "scheduler_base.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

class Scheduler : public SchedulerBase<struct scheduler_bpf> {
public:
  explicit Scheduler(std::unique_ptr<scheduler_bpf> skel = {});

  std::string getName() const override;
};

} // namespace scheduler
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
