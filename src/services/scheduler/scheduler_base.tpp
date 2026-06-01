#pragma once

// clang-format off
#include "scheduler_base.h"
#include "src/common/assert.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

template <typename SkelType>
SchedulerBase<SkelType>::SchedulerBase(std::unique_ptr<SkelType> skel)
    : ebpf::EbpfWithNetHook<SkelType>(std::move(skel)) {}

/**
 * @brief 启动调度器
 *
 * @param io_ctx io 模块
 * @return true 成功
 * @return false 失败
 */
template <typename SkelType>
bool SchedulerBase<SkelType>::start(std::weak_ptr<io::IoIf> io_ctx) {
  if (!ebpf::EbpfSkelIf<SkelType>::start(io_ctx)) {
    return false;
  }

  this->open();
  if (this->skel_ == nullptr) {
    return false;
  }

  this->load();
  this->attach();

  return true;
}

/**
 * @brief 停止调度器
 *
 */
template <typename SkelType>
void SchedulerBase<SkelType>::stop() {
  ebpf::EbpfWithNetHook<SkelType>::detach();
  ebpf::EbpfSkelIf<SkelType>::detach();

  this->destroy();
}

/**
 * @brief 获取调度器程序 fd
 *
 * @return int fd
 */
template <typename SkelType>
int SchedulerBase<SkelType>::getMainProgFd() const {
  if constexpr (std::is_same_v<SkelType, struct scheduler_xdp_bpf>) {
    return this->skel_ ? bpf_program__fd(this->skel_->progs.hebpf_scheduler_xdp_gen) : -1;
  } else {
    return this->skel_ ? bpf_program__fd(this->skel_->progs.hebpf_scheduler_tc_ingress) : -1;
  }
}

/**
 * @brief Scheduler 绑定 hook
 * @note 因为 Scheduler 不会有配置文件，所以这里基类方法的实现是直接进行绑定
 *
 * @param hook_type hook 类型
 * @param ifindex 网卡索引
 * @param config Scheduler 配置（无意义）
 */
template <typename SkelType>
void SchedulerBase<SkelType>::onConfigUpdate(daemon::HookType hook_type, int ifindex,
                                             [[maybe_unused]] const nlohmann::json &config) {
  hebpf::ebpf::EbpfWithNetHook<SkelType>::attachGeneric(hook_type, ifindex);
}

} // namespace scheduler
} // namespace services
} // namespace hebpf
