// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_tracing.h> // 提供 BPF_PROG 宏
#include "src/services/klog/klog_helpers.bpf.h"
#include "runqlat.bpf.h"
#include "runqlat_comm.h"
// clang-format on

// 参考 https://github.com/bpftrace/bpftrace/blob/master/tools/runqlat.bt
// BPF 提供 TASK_RUNNING 的办法就是自己定义
#define TASK_RUNNING 0

// 任务被唤醒时记录时间戳
SEC("raw_tp/sched_wakeup")
int BPF_PROG(hebpf_runqlat_sched_wakeup, struct task_struct *task) {
  pid_t pid = BPF_CORE_READ(task, pid);

  // 记录当前时间戳
  __u64 now = bpf_ktime_get_ns();
  bpf_map_update_elem(&hebpf_rq_cputime, &pid, &now, BPF_NOEXIST);

  return 0;
}

// 新任务首次被唤醒
SEC("raw_tp/sched_wakeup_new")
int BPF_PROG(hebpf_runqlat_sched_wakeup_new, struct task_struct *task) {
  pid_t pid = BPF_CORE_READ(task, pid);

  __u64 now = bpf_ktime_get_ns();
  bpf_map_update_elem(&hebpf_rq_cputime, &pid, &now, BPF_NOEXIST);

  return 0;
}

// 获取进程的 TGID
static __always_inline pid_t get_tgid(struct task_struct *task) {
  struct task_struct *group_leader = BPF_CORE_READ(task, group_leader);
  return BPF_CORE_READ(group_leader, pid);
}

// 上下文切换时计算延迟
SEC("raw_tp/sched_switch")
int BPF_PROG(hebpf_runqlat_sched_switch, bool preempt, struct task_struct *prev,
             struct task_struct *next) {

  pid_t next_pid = BPF_CORE_READ(next, pid); // 即将上处理器
  pid_t prev_pid = BPF_CORE_READ(prev, pid); // 正在运行
  __u64 switch_ts = bpf_ktime_get_ns();

  // 从 map 中查找当前任务被唤醒的时间戳
  __u64 *wakeup_ts = bpf_map_lookup_elem(&hebpf_rq_cputime, &next_pid);

  if (wakeup_ts != NULL) {
    __u64 latency = switch_ts - *wakeup_ts;

    __u32 cfg_key = INDEX0_LATENCY_THRESHOLD_NS;
    __u64 *cfg = bpf_map_lookup_elem(&hebpf_rq_config, &cfg_key);
    __u64 latency_threshold_ns = cfg != NULL && *cfg != 0 ? *cfg : LATENCY_THRESHOLD_NS;

    // 只有当延迟超过阈值时才上报
    if (latency >= latency_threshold_ns) {

      struct rq_event *eve = bpf_ringbuf_reserve(&hebpf_rq_rb, sizeof(*eve), 0);
      if (eve != NULL) {
        eve->pid = next_pid;
        eve->tgid = (__u32)get_tgid(next);
        eve->prev_pid = prev_pid;
        eve->prev_tgid = (__u32)get_tgid(prev);
        eve->wakeup_ts = *wakeup_ts;
        eve->switch_ts = switch_ts;
        eve->latency_ns = latency;

        // 复制进程名称
        BPF_CORE_READ_STR_INTO(&eve->comm, next, comm);
        BPF_CORE_READ_STR_INTO(&eve->prev_comm, prev, comm);

        KLOG(KLOG_LEVEL_DEBUG, "Runq(%u) submit event: %u -> %u", latency_threshold_ns,
             eve->prev_pid, eve->pid);
        bpf_ringbuf_submit(eve, 0);
      }
    } // end if( latency )

    // next 是即将上处理器的进程，之前 wakeup 是进入 run queue，现在 switch 是离开 run queue
    // 因此需要在这里（离开 run queue）的时候删除 map 中的记录
    bpf_map_delete_elem(&hebpf_rq_cputime, &next_pid);
  } // end if( wakeup )

  // 如果 prev 任务被抢占（state == TASK_RUNNING），记录是谁抢占了它
  __s64 prev_state = BPF_CORE_READ(prev, __state);
  if (prev_state == TASK_RUNNING) {
    bpf_map_update_elem(&hebpf_rq_preempt, &prev_pid, &next_pid, BPF_ANY);
  }

  return 0;
}

char LICENSE[] SEC("license") = "GPL";
