#pragma once

#define TASK_COMM_LEN 16

#ifdef __cplusplus
extern "C" {
#endif

struct execve_event {
  uint64_t count; // 系统调用次数
};

#ifdef __cplusplus
}
#endif
