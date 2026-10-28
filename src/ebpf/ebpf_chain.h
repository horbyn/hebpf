#pragma once

// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include "ebpf_common.h"
// clang-format on

#ifndef THIS_ID
#error "THIS_ID must be defined"
#endif

// 内核态程序数组，用于尾调用
struct {
  __uint(type, BPF_MAP_TYPE_PROG_ARRAY);
  __uint(max_entries, MAX_CHAIN_PROGS);
  __type(key, __u32);
  __type(value, __u32);
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} hebpf_parray SEC(".maps");

// 内核态哈希表，运行时标识映射程序数组下标
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, MAX_CHAIN_PROGS);
  __type(key, __u64);
  __type(value, __u32);
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} hebpf_idhash SEC(".maps");

#define CHAIN_NEXT(ctx, hook, other_id)                                                            \
  do {                                                                                             \
    __u64 key = CHAIN_KEY(hook, other_id, THIS_ID);                                                \
    __u32 *idx = bpf_map_lookup_elem(&hebpf_idhash, &key);                                         \
    if (idx) {                                                                                     \
      __u32 next = *idx + 1;                                                                       \
      if (next < MAX_CHAIN_PROGS) {                                                                \
        bpf_tail_call(ctx, &hebpf_parray, next);                                                   \
      }                                                                                            \
    }                                                                                              \
  } while (0)
