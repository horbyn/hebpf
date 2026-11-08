// clang-format off
#include "scheduler_sched_switch.h"
#include "generated/ebpf_prog_map.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

SchedulerSchedSwitch::SchedulerSchedSwitch(std::unique_ptr<scheduler_sched_switch_bpf> skel)
    : ebpf::EbpfSkelIf<scheduler_sched_switch_bpf>(std::move(skel)) {}

/**
 * @brief 获取调度器程序名称
 *
 * @return std::string 程序名称
 */
std::string SchedulerSchedSwitch::getName() const { return std::string{SCHEDULER_Sched}; }

/**
 * @brief 加载 scheduler eBPF 程序
 *
 * @note 加载失败会抛出异常
 */
void SchedulerSchedSwitch::load() {
  switch (type_) {
  case daemon::HookType::Sched_Switch:
    bpf_program__set_autoload(skel_->progs.hebpf_scheduler_sched_switch, true);
    break;
  case daemon::HookType::Sched_Wakeup:
    bpf_program__set_autoload(skel_->progs.hebpf_scheduler_sched_wakeup, true);
    break;
  default:
    bpf_program__set_autoload(skel_->progs.hebpf_scheduler_sched_wakeup_new, true);
    break;
  }
  ebpf::EbpfSkelIf<scheduler_sched_switch_bpf>::load();
}

/**
 * @brief 获取 Tracepoint 调度器 fd
 * @param hook_type 绑定类型
 *
 * @return int fd
 */
int SchedulerSchedSwitch::getRunningFd(daemon::HookType hook_type) const {
  if (this->skel_ == nullptr) {
    return FD_INVALID;
  }
  switch (hook_type) {
  case daemon::HookType::Sched_Switch:
    return bpf_program__fd(this->skel_->progs.hebpf_scheduler_sched_switch);
  case daemon::HookType::Sched_Wakeup:
    return bpf_program__fd(this->skel_->progs.hebpf_scheduler_sched_wakeup);
  default:
    return bpf_program__fd(this->skel_->progs.hebpf_scheduler_sched_wakeup_new);
  }
}

} // namespace scheduler
} // namespace services
} // namespace hebpf
