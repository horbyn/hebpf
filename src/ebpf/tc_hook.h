#pragma once

// clang-format off
#include <bpf/libbpf.h>
#include "ebpf_net_hook_if.h"
// clang-format on

namespace hebpf {
namespace ebpf {

class TcHook : public EbpfNetHookIf {
public:
  enum class Direct : uint8_t { INGRESS, EGRESS };

  explicit TcHook(int ifindex, Direct direct);

  int do_attach(int prog_fd) override;
  int do_detach() override;
  int pre_attach_check() override;
  void post_detach_cleanup() override;

private:
  void setBpfTcHook(struct bpf_tc_hook *hook, int ifindex, Direct direct, uint32_t parent) const;

  Direct direct_;
  struct bpf_tc_hook hook_;
  bool hook_created_;
  uint32_t handle_;
  uint32_t priority_;
};

} // namespace ebpf
} // namespace hebpf
