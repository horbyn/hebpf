// clang-format off
#include "scheduler_xdp.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

SchedulerXdp::SchedulerXdp(std::unique_ptr<scheduler_xdp_bpf> skel)
    : SchedulerBase(std::move(skel)) {}

/**
 * @brief 获取调度器程序名称
 *
 * @return std::string 程序名称
 */
std::string SchedulerXdp::getName() const { return std::string{SERVICE_NAME_SCHEDULER}; }

} // namespace scheduler
} // namespace services
} // namespace hebpf

/**
 * @brief 动态库导出接口：用来返回 eBPF 对象
 *
 * @return std::unique_ptr<hebpf::ebpf::EbpfIf> 对象
 */
extern "C" std::unique_ptr<hebpf::ebpf::EbpfIf> create_service() {
  return std::make_unique<hebpf::services::scheduler::SchedulerXdp>();
}
