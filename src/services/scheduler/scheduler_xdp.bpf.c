// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include "src/ebpf/ebpf_chain.h"
// clang-format on

SEC("xdp")
int hebpf_scheduler_xdp_gen(struct xdp_md *ctx) {

#if defined(THIS_ID)
  __u32 ifindex = ctx->ingress_ifindex;
  CHAIN_NEXT(ctx, XDP_Generic, ifindex);
#endif

  return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
