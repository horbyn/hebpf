#pragma once

// clang-format off
#include "generated/ebpf_hook_type.h"
// clang-format on

#define MAX_HOOK_TYPE 128    // 最多支持的 hook 类型数量
#define MAX_PER_HOOK_PROGS 8 // 每个 hook 最多支持的 eBPF 程序数量
#define MAX_CHAIN_PROGS ((MAX_HOOK_TYPE) * (MAX_PER_HOOK_PROGS))
#define PATH_BPFFS "/sys/fs/bpf/"

/**
 * @brief 构造 idhash 的 key（64 位）
 * @param hook_type 绑定类型（枚举值）
 * @param other_id 其他标识（如网络 eBPF 程序可以填入 NIC 索引）
 * @param prog_id 程序标识（来自 ebpf_id.map 的 ID）
 *
 * @note hook_type 数量由枚举值限制，考虑扩展定义 16-bit
 * @note other_id 数量由 MAX_HOOK_TYPE 限制，目前是 4，定义 16-bit
 * @note prog_id 数量为 hebpf 支持的所有程序的数量，考虑扩展定义 32-bit
 *
 * @return 64 位无符号整数：高 16 位为绑定类型，中间 16 位为绑定标识，低 32 位为程序 ID
 */
#define CHAIN_KEY(hook_type, other_id, prog_id)                                                    \
  (((unsigned long long int)((HOOK_TYPE_VAL(hook_type)) & 0xFFFF) << 48) |                         \
   ((unsigned long long int)((other_id) & 0xFFFF) << 32) | ((unsigned int)(prog_id)))
