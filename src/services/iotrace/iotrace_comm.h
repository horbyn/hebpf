#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 事件类型
#define IO_TYPE_Q2C 0
#define IO_TYPE_D2C 1
#define IO_TYPE_I2D 2

// 上报给用户态的 I/O 事件结构
struct hebpf_io_event {
  unsigned long long latency_ns; // 延迟时间（纳秒）
  unsigned int type;             // 延迟类型 (Q2C, D2C, I2D)
  unsigned int dev;              // 设备号
  unsigned long long sector;     // 扇区号
  unsigned long long bytes;      // I/O 大小
  char comm[16];                 // 触发 I/O 的进程名
};

#ifdef __cplusplus
}
#endif
