// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h> // 提供 BPF_PROG 宏
#include "src/ebpf/ebpf_chain.h"
// clang-format on

SEC("raw_tp/sched_wakeup")
int BPF_PROG(hebpf_scheduler_sched_wakeup, bool preempt, struct task_struct *prev,
             struct task_struct *next) {
#ifdef THIS_ID
  CHAIN_NEXT(ctx, Sched_Wakeup, 0);
#endif
  return 0;
}

SEC("raw_tp/sched_wakeup_new")
int BPF_PROG(hebpf_scheduler_sched_wakeup_new, bool preempt, struct task_struct *prev,
             struct task_struct *next) {
#ifdef THIS_ID
  CHAIN_NEXT(ctx, Sched_Wakeup_New, 0);
#endif
  return 0;
}

SEC("raw_tp/sched_switch")
int BPF_PROG(hebpf_scheduler_sched_switch, bool preempt, struct task_struct *prev,
             struct task_struct *next) {
#ifdef THIS_ID
  CHAIN_NEXT(ctx, Sched_Switch, 0);
#endif
  return 0;
}

char _license[] SEC("license") = "GPL";
