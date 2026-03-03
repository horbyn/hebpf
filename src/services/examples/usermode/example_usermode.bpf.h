#pragma once

#define TASK_COMM_LEN 16

#ifdef __cplusplus
extern "C" {
#endif

struct execve_event {
  int pid;                  // 进程ID
  char comm[TASK_COMM_LEN]; // 进程名
};

#ifdef __cplusplus
}
#endif
