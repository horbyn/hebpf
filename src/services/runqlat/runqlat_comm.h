#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PROG_NAME 16
#define LATENCY_THRESHOLD_NS 10000000ULL // 延迟阈值（纳秒），默认 10ms = 10,000,000 ns
#define INDEX0_LATENCY_THRESHOLD_NS 0

// 通过 ring_buffer 从内核态发送到用户态的事件
struct rq_event {
  unsigned int pid;                  // 进程 PID
  unsigned int tgid;                 // 进程 TGID (线程组 ID)
  unsigned int prev_pid;             // 被抢占的进程 PID
  unsigned int prev_tgid;            // 被抢占的进程 TGID
  unsigned long long int wakeup_ts;  // 任务进入 run queue 的时间戳 (ns)
  unsigned long long int switch_ts;  // 任务获得 CPU 的时间戳 (ns)
  unsigned long long int latency_ns; // 进入 run queue 的等待时间（延迟） (ns)
  char comm[MAX_PROG_NAME];          // 进程名称
  char prev_comm[MAX_PROG_NAME];     // 被抢占进程名称
};

#ifdef __cplusplus
}
#endif
