// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include "src/ebpf/ebpf_chain.h"
// clang-format on

#define TC_ACT_OK 0

SEC("tc")
int hebpf_scheduler_tc_ingress(struct __sk_buff *skb) {

#ifdef THIS_ID
  __u32 ifindex = skb->ifindex;
  CHAIN_NEXT(skb, ifindex);
#endif

  return TC_ACT_OK;
}

char _license[] SEC("license") = "GPL";
