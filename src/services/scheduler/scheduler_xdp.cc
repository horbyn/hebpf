// clang-format off
#include "scheduler_xdp.h"
#include "src/daemon/hooks.h"
#include "generated/ebpf_prog_map.h"
// clang-format on

namespace hebpf {
namespace services {
namespace scheduler {

SchedulerXdp::SchedulerXdp(std::unique_ptr<scheduler_xdp_bpf> skel)
    : ebpf::EbpfWithNetHook<scheduler_xdp_bpf>(std::move(skel)) {}

/**
 * @brief 获取 XDP 调度器 fd
 * @param hook_type 绑定类型
 *
 * @return int fd
 */
int SchedulerXdp::getRunningFd([[maybe_unused]] daemon::HookType hook_type) const {
  return this->skel_ ? bpf_program__fd(this->skel_->progs.hebpf_scheduler_xdp_gen) : FD_INVALID;
}

/**
 * @brief XDP 调度器 绑定 hook
 *
 * @param config 调度器配置
 *
 * @return true 修改成功，false 修改失败
 */
bool SchedulerXdp::onConfigUpdate(const nlohmann::json &config) {
  daemon::HookKey hook_key = config;
  return ebpf::EbpfWithNetHook<scheduler_xdp_bpf>::attachGeneric(
      hook_key.hook, static_cast<int>(hook_key.hook_id));
}

/**
 * @brief 获取调度器程序名称
 *
 * @return std::string 程序名称
 */
std::string SchedulerXdp::getName() const { return std::string{SCHEDULER_XDP_Generic}; }

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
