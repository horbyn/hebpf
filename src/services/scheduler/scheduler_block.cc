// clang-format off
#include "scheduler_block.h"
#include "generated/ebpf_prog_map.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

SchedulerBlock::SchedulerBlock(std::unique_ptr<scheduler_block_bpf> skel)
    : ebpf::EbpfSkelIf<scheduler_block_bpf>(std::move(skel)) {}

/**
 * @brief 获取调度器程序名称
 *
 * @return std::string 程序名称
 */
std::string SchedulerBlock::getName() const { return std::string{SCHEDULER_Block}; }

/**
 * @brief 加载 scheduler eBPF 程序
 *
 * @note 加载失败会抛出异常
 */
void SchedulerBlock::load() {
  switch (type_) {
  case daemon::HookType::Block_Bio_Queue:
    bpf_program__set_autoload(skel_->progs.hebpf_scheduler_block_bio_queue, true);
    break;
  case daemon::HookType::Block_Io_Start:
    bpf_program__set_autoload(skel_->progs.hebpf_scheduler_block_io_start, true);
    break;
  case daemon::HookType::Block_Rq_Issue:
    bpf_program__set_autoload(skel_->progs.hebpf_scheduler_block_rq_issue, true);
    break;
  default:
    bpf_program__set_autoload(skel_->progs.hebpf_scheduler_block_rq_complete, true);
    break;
  }
  ebpf::EbpfSkelIf<scheduler_block_bpf>::load();
}

/**
 * @brief 获取 Tracepoint 调度器 fd
 * @param hook_type 绑定类型
 *
 * @return int fd
 */
int SchedulerBlock::getRunningFd(daemon::HookType hook_type) const {
  if (this->skel_ == nullptr) {
    return FD_INVALID;
  }
  switch (hook_type) {
  case daemon::HookType::Block_Bio_Queue:
    return bpf_program__fd(skel_->progs.hebpf_scheduler_block_bio_queue);
  case daemon::HookType::Block_Io_Start:
    return bpf_program__fd(skel_->progs.hebpf_scheduler_block_io_start);
  case daemon::HookType::Block_Rq_Issue:
    return bpf_program__fd(skel_->progs.hebpf_scheduler_block_rq_issue);
  default:
    return bpf_program__fd(skel_->progs.hebpf_scheduler_block_rq_complete);
  }
}

} // namespace scheduler
} // namespace services
} // namespace hebpf
