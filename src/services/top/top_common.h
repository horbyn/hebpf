#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// CPU 统计信息
struct top_cpu_time {
  unsigned long long user_ns;   // user CPU time
  unsigned long long system_ns; // system CPU time
  unsigned long long idle_ns;   // idle time
};

#define INDEX_CPU_USAGE 0
#define INDEX_CPU_LAST 0
#define MAX_TASKS 10240

#ifdef __cplusplus
}
#endif
