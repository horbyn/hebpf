#pragma once

// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
// clang-format on

#define MAX_TASK_ENTRIES 10240

// 用于向用户态传递事件
struct {
  __uint(type, BPF_MAP_TYPE_RINGBUF);
  __uint(max_entries, 256 * 1024); // 256KB
} hebpf_rq_rb SEC(".maps");

// 存储任务被唤醒的时间戳 (pid -> timestamp)
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, MAX_TASK_ENTRIES);
  __type(key, __u32);   // PID
  __type(value, __u64); // 累计 CPU 纳秒数
} hebpf_rq_cputime SEC(".maps");

// 存储被抢占进程的信息 (pid -> prev_pid)
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, MAX_TASK_ENTRIES);
  __type(key, __u32);
  __type(value, __u32);
} hebpf_rq_preempt SEC(".maps");

// 配置
struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __uint(max_entries, 1);
  __type(key, __u32);
  __type(value, __u64);
} hebpf_rq_config SEC(".maps");
