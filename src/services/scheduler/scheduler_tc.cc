// clang-format off
#include "scheduler_tc.h"
#include "src/daemon/hooks.h"
#include "generated/ebpf_prog_map.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

SchedulerTc::SchedulerTc(std::unique_ptr<scheduler_tc_bpf> skel)
    : ebpf::EbpfWithNetHook<scheduler_tc_bpf>(std::move(skel)) {}

/**
 * @brief 获取调度器程序名称
 *
 * @return std::string 程序名称
 */
std::string SchedulerTc::getName() const { return std::string{SCHEDULER_TC}; }

/**
 * @brief 获取 TC 调度器 fd
 * @param hook_type 绑定类型
 *
 * @return int fd
 */
int SchedulerTc::getRunningFd([[maybe_unused]] daemon::HookType hook_type) const {
  return this->skel_ ? bpf_program__fd(this->skel_->progs.hebpf_scheduler_tc_ingress) : FD_INVALID;
}

/**
 * @brief TC 调度器 绑定 hook
 *
 * @param config 调度器配置
 *
 * @return true 修改成功，false 修改失败
 */
bool SchedulerTc::onConfigUpdate(const nlohmann::json &config) {
  daemon::HookKey hook_key = config;
  return ebpf::EbpfWithNetHook<scheduler_tc_bpf>::attachGeneric(hook_key.hook,
                                                                static_cast<int>(hook_key.hook_id));
}

} // namespace scheduler
} // namespace services
} // namespace hebpf

/**
 * @brief 动态库导出接口：用来返回 eBPF 对象
 *
 * @return std::unique_ptr<hebpf::ebpf::EbpfIf> 对象
 */
extern "C" std::unique_ptr<hebpf::ebpf::EbpfIf> create_service() {
  return std::make_unique<hebpf::services::scheduler::SchedulerTc>();
}
