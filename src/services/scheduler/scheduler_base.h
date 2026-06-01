#pragma once

// clang-format off
#include "src/ebpf/ebpf_with_net_hook.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

constexpr std::string_view SERVICE_NAME_SCHEDULER{"scheduler"};

template <typename SkelType>
class SchedulerBase : public ebpf::EbpfWithNetHook<SkelType> {
public:
  explicit SchedulerBase(std::unique_ptr<SkelType> skel = {});

  bool start(std::weak_ptr<io::IoIf> io_ctx = {}) override;
  void stop() override;
  int getMainProgFd() const override;
  void onConfigUpdate(daemon::HookType hook_type, int ifindex,
                      const nlohmann::json &config) override;
};

} // namespace scheduler
} // namespace services
} // namespace hebpf

#include "scheduler_base.tpp"

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
