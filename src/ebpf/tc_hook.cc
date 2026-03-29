// clang-format off
#include "tc_hook.h"
// clang-format on

namespace hebpf {
namespace ebpf {

TcHook::TcHook(int ifindex, Direct direct)
    : EbpfNetHookIf{ifindex}, direct_{direct}, hook_{}, hook_created_{false}, handle_{0},
      priority_{0} {
  setBpfTcHook(&hook_, ifindex_, direct_, 0);
}

/**
 * @brief 绑定 TC hook
 *
 * @param prog_fd eBPF 程序文件描述符
 * @return int 成功返回 0，出错返回非零
 */
int TcHook::do_attach(int prog_fd) {
  DECLARE_LIBBPF_OPTS(bpf_tc_opts, tc_opts, .prog_fd = prog_fd, .flags = 0, .prog_id = 0,
                      .handle = 0, .priority = 0);

  int err = bpf_tc_attach(&hook_, &tc_opts);
  if (err == 0) {
    handle_ = tc_opts.handle;
    priority_ = tc_opts.priority;
    LOG(debug, "TC hook attached");
  }

  return err;
}

/**
 * @brief 取消绑定 TC hook
 *
 * @return int 成功返回 0，出错返回非零
 */
int TcHook::do_detach() {
  DECLARE_LIBBPF_OPTS(bpf_tc_opts, tc_opts, .prog_fd = 0, .flags = 0, .prog_id = 0,
                      .handle = handle_, .priority = priority_);
  int err = bpf_tc_detach(&hook_, &tc_opts);
  if (err == 0) {
    LOG(debug, "TC hook detached");
  }

  return err;
}

/**
 * @brief 绑定前检查 clsact qdisc 是否存在
 *
 * @return int 成功返回 0，出错返回非 0
 */
int TcHook::pre_attach_check() {
  int err = bpf_tc_hook_create(&hook_);
  if (err == 0) {
    hook_created_ = true;
    LOG(debug, "TC hook created");
  } else if (err == -EEXIST) {
    hook_created_ = false; // hook 已存在，要么其他程序创建，要么是绑定了 egress
  } else {
    return err;
  }
  return 0;
}

/**
 * @brief 取消绑定后检查 clsact qdisc 是否需要清理
 *
 */
void TcHook::post_detach_cleanup() {
  if (hook_created_) {
    if (bpf_tc_hook_destroy(&hook_) == 0) {
      LOG(debug, "TC hook destroied");
      hook_created_ = false;
    }
  }
}

/**
 * @brief 设置 bpf_tc_hook 对象
 *
 * @param hook 结果
 * @param ifindex 网卡索引
 * @param direct 方向
 * @param parent 父 hook
 */
void TcHook::setBpfTcHook(struct bpf_tc_hook *hook, int ifindex, Direct direct,
                          uint32_t parent) const {
  if (hook == nullptr) {
    return;
  }

  enum bpf_tc_attach_point attach_point;
  switch (direct) {
  case TcHook::Direct::INGRESS:
    attach_point = BPF_TC_INGRESS;
    break;
  case TcHook::Direct::EGRESS:
    attach_point = BPF_TC_EGRESS;
    break;
  default:
    attach_point = BPF_TC_CUSTOM;
    break;
  }

  hook->sz = sizeof(struct bpf_tc_hook);
  hook->ifindex = ifindex;
  hook->attach_point = attach_point;
  hook->parent = parent;
}

} // namespace ebpf
} // namespace hebpf
