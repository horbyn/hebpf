// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "acl.bpf.h"
// clang-format on

#define ANY_PORT 0
#define TC_ACT_OK 0
#define TC_ACT_SHOT 2
#define ETH_P_IP 0x0800

// 内核态 ACL 规则哈希表
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 10240);
  __type(key, struct ipv4_tuple);
  __type(value, __u32);
} krules_acl SEC(".maps");

SEC("tc")
int hebpf_acl_tc_ingress(struct __sk_buff *skb) {

  void *data = (void *)(__u64)skb->data;
  void *data_end = (void *)(__u64)skb->data_end;

  if (skb->protocol != bpf_htons(ETH_P_IP)) {
    return TC_ACT_OK;
  }

  struct ethhdr *eth = data;
  if ((void *)(eth + 1) > data_end) {
    return TC_ACT_OK;
  }

  struct iphdr *ip = (struct iphdr *)(eth + 1);
  if ((void *)(ip + 1) > data_end) {
    return TC_ACT_OK;
  }

  struct ipv4_tuple tuple = {};
  tuple.saddr = ip->saddr;
  tuple.daddr = ip->daddr;
  tuple.protocol = ip->protocol;
  tuple.sport = ANY_PORT;

  uint16_t origin_src_port = 0;
  __u16 offset = ip->ihl * 4;
  if (ip->protocol == IPPROTO_TCP || ip->protocol == IPPROTO_UDP) {
    void *trans = (void *)ip + offset;
    if (trans + sizeof(struct tcphdr) <= data_end) {
      if (ip->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp = trans;
        origin_src_port = tcp->source;
        tuple.dport = tcp->dest;
      }
    } else if (trans + sizeof(struct udphdr) <= data_end) {
      if (ip->protocol == IPPROTO_UDP) {
        struct udphdr *udp = trans;
        origin_src_port = udp->source;
        tuple.dport = udp->dest;
      }
    } else {
      return TC_ACT_OK;
    }
  } else {
    return TC_ACT_OK;
  }

  // 先用任意源端口查找一次
  __u32 *action = bpf_map_lookup_elem(&krules_acl, &tuple);
  if (action == NULL) {
    // 再用原始源端口查找
    tuple.sport = origin_src_port;
    action = bpf_map_lookup_elem(&krules_acl, &tuple);
  }
  if (action && *action == ACL_ACTION_DENY) {
    bpf_printk("Hebpf packet drop");
    return TC_ACT_SHOT;
  }

  bpf_printk("Hebpf packet [%u]: %u:%u -> %u:%u", tuple.protocol, tuple.saddr,
             bpf_ntohs(origin_src_port), tuple.daddr, bpf_ntohs(tuple.dport));
  return TC_ACT_OK;
}

char _license[] SEC("license") = "GPL";
