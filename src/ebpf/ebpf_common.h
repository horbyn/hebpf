#pragma once

#define MAX_NIC_HOOK 4
#define MAX_PER_NIC_PROGS 8
#define MAX_CHAIN_PROGS ((MAX_NIC_HOOK) * (MAX_PER_NIC_PROGS))
#define PATH_BPFFS "/sys/fs/bpf/"

/**
 * @brief 构造 idhash 的 key（64 位）
 * @param ifindex 网络接口索引
 * @param id 程序标识（来自 ebpf_id.map 的 ID）
 * @return 64 位无符号整数：高 32 位为 ifindex，低 32 位为程序 ID
 */
#define CHAIN_KEY(ifindex, id) (((unsigned long long)(ifindex) << 32) | (unsigned int)(id))
