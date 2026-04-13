#pragma once

// clang-format off
#include "klog.bpf.h"
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

struct {
  __uint(type, BPF_MAP_TYPE_RINGBUF);
  __uint(max_entries, KLOG_BUFFER_SIZE);
  __uint(pinning, LIBBPF_PIN_BY_NAME); // 声明为 pinning
} hebpf_klog SEC(".maps");

/**
 * @brief 辅助函数：封装内核态日志结构体，发送日志结构体到 ring buffer
 *
 * @param level 日志等级
 * @param msg 格式化参数
 * @param args 格式化参数
 */
static __always_inline void bpf_log_event_internal(uint16_t level, const char *msg,
                                                   uint64_t args[MAX_ARGS_CNT]) {
  uint16_t msg_len = 0;
#pragma unroll
  for (int i = 0; msg[i]; ++i) {
    ++msg_len;
  }

  size_t total_size = sizeof(struct log_event) + msg_len + 1;
  struct log_event *loge = bpf_ringbuf_reserve(&hebpf_klog, total_size, 0);
  if (loge == NULL) {
    return;
  }

  loge->level = level;
  loge->msg_len = msg_len;

#pragma unroll
  for (int i = 0; i < msg_len; ++i) {
    loge->msg[i] = msg[i];
  }
  loge->msg[msg_len] = '\0';

#pragma unroll
  for (int i = 0; i < MAX_ARGS_CNT; ++i) {
    loge->args[i] = args[i];
  }

  bpf_ringbuf_submit(loge, 0);
}

#ifdef __cplusplus
}
#endif

/**
 * @brief 内核态日志宏
 *
 * @param level 日志等级（KLOG_LEVEL_*）
 * @param fmt 格式化字符串
 * @param ... 格式化参数
 */
#define KLOG(level, fmt, ...)                                                                      \
  do {                                                                                             \
    uint64_t _args[MAX_ARGS_CNT] = {__VA_ARGS__};                                                  \
    bpf_log_event_internal(level, fmt, _args);                                                     \
  } while (0)
