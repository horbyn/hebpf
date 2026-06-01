#pragma once

// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include "ebpf_common.h"
// clang-format on

#ifndef THIS_ID
#error "THIS_ID must be defined"
#endif
#ifndef HOOK_TYPE
#error "HOOK_TYPE must be defined"
#endif

struct prog_value {
  __u32 val;
} __attribute__((packed));

// TC_Ingress
struct {
  __uint(type, BPF_MAP_TYPE_PROG_ARRAY);
  __uint(max_entries, MAX_CHAIN_PROGS);
  __uint(key_size, sizeof(__u32));
  __uint(value_size, sizeof(__u32));
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} TC_In_parray SEC(".maps");
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, MAX_CHAIN_PROGS);
  __type(key, __u64);
  __type(value, __u32);
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} TC_In_idhash SEC(".maps");

// XDP_Generic
struct {
  __uint(type, BPF_MAP_TYPE_PROG_ARRAY);
  __uint(max_entries, MAX_CHAIN_PROGS);
  __uint(key_size, sizeof(__u32));
  __uint(value_size, sizeof(__u32));
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} XDP_Gen_parray SEC(".maps");
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, MAX_CHAIN_PROGS);
  __type(key, __u64);
  __type(value, __u32);
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} XDP_Gen_idhash SEC(".maps");

// 连接宏
#define CONCAT(a, b) a##b
#define XCONCAT(a, b) CONCAT(a, b)
#define CHAIN_NEXT(ctx, ifindex)                                                                   \
  do {                                                                                             \
    __u32 id = THIS_ID;                                                                            \
    __u64 key = CHAIN_KEY(ifindex, id);                                                            \
    __u32 *idx = bpf_map_lookup_elem(&XCONCAT(HOOK_TYPE, _idhash), &key);                          \
    if (idx) {                                                                                     \
      __u32 next = *idx + 1;                                                                       \
      if (next < MAX_CHAIN_PROGS) {                                                                \
        bpf_tail_call(ctx, &XCONCAT(HOOK_TYPE, _parray), next);                                    \
      }                                                                                            \
    }                                                                                              \
  } while (0)
