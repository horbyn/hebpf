// clang-format off
#include "xdp_hook.h"
#include <uapi/linux/if_link.h>
#include "src/common/enum_name.hpp"
// clang-format on

namespace hebpf {
namespace ebpf {

XdpHook::XdpHook(int ifindex, Mode mode)
    : EbpfNetHookIf{ifindex}, mode_{mode}, hook_created_{false} {}

/**
 * @brief 绑定 XDP hook
 *
 * @param prog_fd eBPF 程序文件描述符
 * @return int 成功返回 0，出错返回非零
 */
int XdpHook::do_attach(int prog_fd) {
  int err = bpf_xdp_attach(ifindex_, prog_fd, getFlags(), nullptr);
  if (err == 0) {
    LOG(debug, "{} hook attached successfully", enumName<Mode>(mode_));
  }

  return err;
}

/**
 * @brief 取消绑定 XDP hook
 *
 * @return int 成功返回 0，出错返回非零
 */
int XdpHook::do_detach() {
  int err = bpf_xdp_detach(ifindex_, getFlags(), nullptr);
  if (err == 0) {
    LOG(debug, "{} hook detached successfully", enumName<Mode>(mode_));
  }

  return err;
}

/**
 * @brief 获取标识
 *
 * @return unsigned int XDP mode 标识
 */
unsigned int XdpHook::getFlags() const {
  unsigned int flags{};

  switch (mode_) {
  case Mode::XDP_NATIVE:
    flags = XDP_FLAGS_DRV_MODE;
    LOG(warn, "Attaches to XDP native mode maybe need the hardward supported");
    break;
  case Mode::XDP_OFFLOAD:
    flags = XDP_FLAGS_HW_MODE;
    LOG(warn, "Attaches to XDP offload mode maybe need the hardward supported");
    break;
  default:
    flags = XDP_FLAGS_SKB_MODE;
    break;
  }
  return flags;
}

} // namespace ebpf
} // namespace hebpf
