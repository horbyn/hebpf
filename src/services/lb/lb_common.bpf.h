#pragma once

// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include "lb_common.h"
// clang-format on

#define MAXSIZE_LB_HASH 64

#ifdef __cplusplus
extern "C" {
#endif

// VIP:VPORT -> 后端数组
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, MAXSIZE_LB_HASH);
  __type(key, struct vip_key);
  __type(value, struct vip_group);
} vip_backends SEC(".maps");

// 全局后端池，key 为后端索引 (0 ~ TOTAL_BACKENDS-1)
struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __uint(max_entries, TOTAL_BACKENDS);
  __type(key, __u32);
  __type(value, struct backend_info);
} backend_pool SEC(".maps");

// 本地 MAC 地址
struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __uint(max_entries, 1);
  __type(key, __u32);
  __type(value, __u8[6]);
} lb_lomac SEC(".maps");

#ifdef __cplusplus
}
#endif

#define ETH_ALEN 6
#define ETH_P_IP 0x0800
