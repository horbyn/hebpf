// clang-format off
#include <bpf/bpf_endian.h>
#include "src/ebpf/ebpf_chain.h"
#include "src/services/klog/klog_helpers.bpf.h"
#include "lb_common.bpf.h"
// clang-format on

SEC("xdp")
int hebpf_lb_xdp_gen(struct xdp_md *ctx) {

  void *data = (void *)(__u64)ctx->data;
  void *data_end = (void *)(__u64)ctx->data_end;

  struct ethhdr *eth = data;
  if ((void *)(eth + 1) > data_end) {
    return XDP_PASS;
  }
  if (eth->h_proto != bpf_htons(ETH_P_IP)) {
    return XDP_PASS;
  }

  struct iphdr *ip = (struct iphdr *)(eth + 1);
  if ((void *)(ip + 1) > data_end) {
    return XDP_PASS;
  }

  struct backend_info *be = NULL;
  __u16 dport = 0;
  if (ip->protocol == IPPROTO_TCP) {
    struct tcphdr *tcp = (void *)ip + (ip->ihl * 4);
    if ((void *)(tcp + 1) > data_end) {
      return XDP_PASS;
    }
    dport = tcp->dest;
  } else if (ip->protocol == IPPROTO_UDP) {
    struct udphdr *udp = (void *)ip + (ip->ihl * 4);
    if ((void *)(udp + 1) > data_end) {
      return XDP_PASS;
    }
    dport = udp->dest;
  } else {
    goto chain;
  }

  struct vip_key vkey = {.vip = ip->daddr, .vport = dport, .padding = 0};

  struct vip_group *group = bpf_map_lookup_elem(&vip_backends, &vkey);
  if (group == NULL || group->count == 0) {
    goto chain;
  }

  __u32 src_ip = ip->saddr;
  __u32 idx = src_ip % group->count; // 源地址哈希
  if (idx >= MAX_BACKENDS_PER_VIP) {
    goto chain;
  }
  __u32 pool_idx = group->backends[idx];

  be = bpf_map_lookup_elem(&backend_pool, &pool_idx);
  if (be == NULL) {
    goto chain;
  }

  // 修改源 MAC 为 LB 的本地 MAC
  __u32 k = 0;
  __u8 *lb_src_mac = bpf_map_lookup_elem(&lb_lomac, &k);
  if (lb_src_mac != NULL) {
    __builtin_memcpy(eth->h_source, lb_src_mac, ETH_ALEN);
  } // 即使失败也继续，不影响主要功能，但最好保证 map 已填充

  // DSR：只修改目的 MAC，不修改 IP 和端口
  __builtin_memcpy(eth->h_dest, be->mac, ETH_ALEN);
  KLOG(KLOG_LEVEL_DEBUG, "LB DSR: VIP %A:%u -> backend MAC %x:%x:%x:%x:%x:%x", ip->daddr,
       bpf_ntohs(dport), be->mac[0], be->mac[1], be->mac[2], be->mac[3], be->mac[4], be->mac[5]);

chain: {
#ifdef THIS_ID
  __u32 ifindex = ctx->ingress_ifindex;
  CHAIN_NEXT(ctx, ifindex);
#endif
}

  if (be != NULL) {
    return XDP_TX; // 此时目的地址已修改, 相当于单臂 LB
  }
  return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
