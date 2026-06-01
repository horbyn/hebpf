#pragma once

#define MAX_BACKENDS_PER_VIP 8 // 每个 VIP:VPORT 最多支持的后端数
#define TOTAL_BACKENDS 256     // 后端池总容量

#ifdef __cplusplus
extern "C" {
#endif

struct vip_key {
  unsigned int vip;       // VIP 地址（网络字节序）
  unsigned short vport;   // VIP 端口（网络字节序）
  unsigned short padding; // 对齐填充，使 key 为 8 字节
} __attribute__((packed));

struct vip_group {
  unsigned int backends[MAX_BACKENDS_PER_VIP];
  unsigned int count; // 有效后端数量
  unsigned int padding;
} __attribute__((packed));

struct backend_info {
  unsigned char mac[6];   // 后端 MAC 地址
  unsigned short padding; // 对齐
} __attribute__((packed));

#ifdef __cplusplus
}
#endif
