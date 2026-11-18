// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_tracing.h>
#include "src/ebpf/ebpf_chain.h"
#include "src/services/klog/klog_helpers.bpf.h"
#include "top_common.h"
// clang-format on

#define PID_IDLE 0

// 独立记录每个逻辑核心的 on-CPU time
struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __uint(max_entries, 1);
  __type(key, __u32);
  __type(value, struct top_cpu_time);
} hebpf_top_cputm SEC(".maps");

// 每个任务的最后一次 on-CPU time 快照
struct top_task_snapshot {
  __u64 timestamp_ns;
  __u64 utime;
  __u64 stime;
};

// 记录每个任务的 on-CPU time （用来计算 user、system 时间）
// 所有逻辑核心共享
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, MAX_TASKS);
  __type(key, __u32); // pid
  __type(value, struct top_task_snapshot);
} hebpf_top_tasktm SEC(".maps");

struct top_idle_state {
  __u64 last_switch_ts;
};

// 每个逻辑核心上次切换任务的时刻，用于计算 idle 时间
struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __uint(max_entries, 1);
  __type(key, __u32);
  __type(value, struct top_idle_state);
} hebpf_top_idletm SEC(".maps");

SEC("raw_tp/sched_switch")
int BPF_PROG(hebpf_top_sched_switch, bool preempt, struct task_struct *prev,
             struct task_struct *next, unsigned int prev_state) {
  __u64 now = bpf_ktime_get_ns();
  __u32 index_cputm = INDEX_CPU_USAGE;
  __u32 index_cpulast = INDEX_CPU_LAST;
  __u32 cpu_id = bpf_get_smp_processor_id();

  struct top_cpu_time *stat = bpf_map_lookup_elem(&hebpf_top_cputm, &index_cputm);
  if (stat == NULL) {
    KLOG(KLOG_LEVEL_WARN, "[Top] Could not get on-CPU usage on core %u", cpu_id);
    goto chain;
  }

  __u32 prev_pid = BPF_CORE_READ(prev, pid);

  // 当前 CPU 的上次 switch 时刻（PERCPU_ARRAY，每个 CPU 访问自己的副本）
  struct top_idle_state *istate = bpf_map_lookup_elem(&hebpf_top_idletm, &index_cpulast);
  if (istate == NULL) {
    KLOG(KLOG_LEVEL_WARN, "[Top] Could not get on-CPU idle time");
    goto chain;
  }

  // 处理 prev（即将切出的任务）
  if (prev_pid == PID_IDLE) {
    stat->idle_ns += now - istate->last_switch_ts;
  } else {
    __u64 prev_utime = BPF_CORE_READ(prev, utime);
    __u64 prev_stime = BPF_CORE_READ(prev, stime);

    struct top_task_snapshot *snap = bpf_map_lookup_elem(&hebpf_top_tasktm, &prev_pid);
    if (snap != NULL) {
      stat->user_ns += prev_utime - snap->utime;
      stat->system_ns += prev_stime - snap->stime;
    }

    // 更新快照（插入或覆盖）
    struct top_task_snapshot new_snap = {
        .timestamp_ns = now,
        .utime = prev_utime,
        .stime = prev_stime,
    };
    bpf_map_update_elem(&hebpf_top_tasktm, &prev_pid, &new_snap, BPF_ANY);
  } // end if-else(prev)

  istate->last_switch_ts = now; // 每次 switch 都更新最后一次时间戳

  __u32 next_pid = BPF_CORE_READ(next, pid);

  // 处理 next（即将执行的任务）
  if (next_pid != PID_IDLE) {
    struct top_task_snapshot next_snap = {
        .timestamp_ns = now,
        .utime = BPF_CORE_READ(next, utime),
        .stime = BPF_CORE_READ(next, stime),
    };
    bpf_map_update_elem(&hebpf_top_tasktm, &next_pid, &next_snap, BPF_ANY);
  }

chain:
#if defined(THIS_ID)
  CHAIN_NEXT(ctx, Sched_Switch, 0);
#endif

  return 0;
}

char LICENSE[] SEC("license") = "GPL";
