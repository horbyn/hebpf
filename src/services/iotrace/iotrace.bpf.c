// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "src/ebpf/ebpf_chain.h"
#include "iotrace_comm.h"
// clang-format on

#define MAX_TRACE_ENTRIES 10240
#define SECTOR_BYTES 512

// 以 (dev, sector) 作为唯一标识
struct io_key {
  __u32 dev;
  __u64 sector;
};

// 记录不同阶段的时间戳
struct start_times {
  __u64 q_ts; // block_bio_queue 时间戳
  __u64 i_ts; // block_io_start 时间戳
  __u64 d_ts; // block_rq_issue 时间戳
};

// 存放起始时间的哈希表
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, MAX_TRACE_ENTRIES);
  __type(key, struct io_key);
  __type(value, struct start_times);
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} hebpf_iotrc_stm SEC(".maps");

// 用于向用户态发送事件的 Ring Buffer
struct {
  __uint(type, BPF_MAP_TYPE_RINGBUF);
  __uint(max_entries, 256 * 1024); // 256KB
  __uint(pinning, LIBBPF_PIN_BY_NAME);
} hebpf_iotrc_rb SEC(".maps");

/**
 * @brief 从 bio 中提取设备号和扇区号
 *
 * @param bio 内核对象
 * @param dev 设备号（返回值）
 * @param sector 扇区号（返回值）
 */
static __always_inline void get_bio_info(struct bio *bio, __u32 *dev, __u64 *sector) {
  *dev = BPF_CORE_READ(bio, bi_bdev, bd_dev);
  *sector = BPF_CORE_READ(bio, bi_iter.bi_sector);
}

/**
 * @brief 从 request 中提取设备号和扇区号
 *
 * @param rq 内核对象
 * @param dev 设备号（返回值）
 * @param sector 扇区号（返回值）
 */
static __always_inline void get_rq_info(struct request *rq, __u32 *dev, __u64 *sector) {
  struct gendisk *disk = BPF_CORE_READ(rq, q, disk);
  if (disk != NULL) {
    // 读取 major 和 first_minor
    __u32 major = BPF_CORE_READ(disk, major);
    __u32 first_minor = BPF_CORE_READ(disk, first_minor);

    // 按照 Linux 内核的 MKDEV 宏定义拼接 dev_t (MINORBITS 为 20)
    *dev = (major << 20) | (first_minor & 0xFFFFF);
  } else {
    *dev = 0;
  }
  *sector = BPF_CORE_READ(rq, __sector);
}

// Q 阶段：应用提交 bio 进队列
SEC("?raw_tp/block_bio_queue")
int BPF_PROG(hebpf_iotrace_block_bio_queue, struct bio *bio) {
  if (bio == NULL) {
    return 0;
  }

  __u32 dev;
  __u64 sector;
  get_bio_info(bio, &dev, &sector);

  struct io_key key = {.dev = dev, .sector = sector};
  struct start_times times = {.q_ts = bpf_ktime_get_ns()};

  // 更新或插入，保留原有的其他时间戳（如果有）
  bpf_map_update_elem(&hebpf_iotrc_stm, &key, &times, BPF_NOEXIST);

#if defined(THIS_ID)
  CHAIN_NEXT(ctx, Block_Bio_Queue, 0);
#endif
  return 0;
}

// I 阶段：bio 转为 request，进入调度器
SEC("?raw_tp/block_io_start")
int BPF_PROG(hebpf_iotrace_block_io_start, struct request *rq) {
  if (rq == NULL) {
    return 0;
  }

  __u32 dev;
  __u64 sector;
  get_rq_info(rq, &dev, &sector);
  __u64 now = bpf_ktime_get_ns();

  struct io_key key = {.dev = dev, .sector = sector};
  struct start_times *times = bpf_map_lookup_elem(&hebpf_iotrc_stm, &key);
  if (times != NULL) {
    times->i_ts = now;
  } else {
    struct start_times new_times = {.i_ts = now};
    bpf_map_update_elem(&hebpf_iotrc_stm, &key, &new_times, BPF_NOEXIST);
  }

#if defined(THIS_ID)
  CHAIN_NEXT(ctx, Block_Io_Start, 0);
#endif
  return 0;
}

// D 阶段：请求派发给设备驱动
SEC("?raw_tp/block_rq_issue")
int BPF_PROG(hebpf_iotrace_block_rq_issue, struct request *rq) {
  if (rq == NULL) {
    return 0;
  }

  __u32 dev;
  __u64 sector;
  get_rq_info(rq, &dev, &sector);
  __u64 now = bpf_ktime_get_ns();

  struct io_key key = {.dev = dev, .sector = sector};
  struct start_times *times = bpf_map_lookup_elem(&hebpf_iotrc_stm, &key);
  if (times != NULL) {
    // 计算 I2D (Insert -> Dispatch)
    if (times->i_ts > 0) {
      __u64 i2d = now - times->i_ts;
      struct hebpf_io_event *eve = bpf_ringbuf_reserve(&hebpf_iotrc_rb, sizeof(*eve), 0);
      if (eve != NULL) {
        eve->latency_ns = i2d;
        eve->type = IO_TYPE_I2D;
        eve->dev = dev;
        eve->sector = sector;
        eve->bytes = BPF_CORE_READ(rq, __data_len); // request 的数据长度
        bpf_get_current_comm(&eve->comm, sizeof(eve->comm));
        bpf_ringbuf_submit(eve, 0);
      } // end if(eve)
    }
    // 记录 D 阶段时间戳
    times->d_ts = now;
  } else {
    struct start_times new_times = {.d_ts = now};
    bpf_map_update_elem(&hebpf_iotrc_stm, &key, &new_times, BPF_NOEXIST);
  }

#if defined(THIS_ID)
  CHAIN_NEXT(ctx, Block_Rq_Issue, 0);
#endif
  return 0;
}

// C 阶段：设备驱动报告完成
SEC("?raw_tp/block_rq_complete")
int BPF_PROG(hebpf_iotrace_block_rq_complete, struct request *rq, blk_status_t error,
             unsigned int nr_bytes) {
  if (rq == NULL) {
    return 0;
  }

  __u32 dev;
  __u64 sector;
  get_rq_info(rq, &dev, &sector);
  __u64 now = bpf_ktime_get_ns();

  struct io_key key = {.dev = dev, .sector = sector};
  struct start_times *times = bpf_map_lookup_elem(&hebpf_iotrc_stm, &key);
  if (times != NULL) {
    // 计算 D2C (Dispatch -> Complete)
    if (times->d_ts > 0) {
      __u64 d2c = now - times->d_ts;
      struct hebpf_io_event *eve = bpf_ringbuf_reserve(&hebpf_iotrc_rb, sizeof(*eve), 0);
      if (eve != NULL) {
        eve->latency_ns = d2c;
        eve->type = IO_TYPE_D2C;
        eve->dev = dev;
        eve->sector = sector;
        eve->bytes = nr_bytes; // 使用 tracepoint 参数中提供的完成字节数
        bpf_get_current_comm(&eve->comm, sizeof(eve->comm));
        bpf_ringbuf_submit(eve, 0);
      } // end if(eve)
    }

    // 计算 Q2C (Queue -> Complete)
    if (times->q_ts > 0) {
      __u64 q2c = now - times->q_ts;
      struct hebpf_io_event *eve = bpf_ringbuf_reserve(&hebpf_iotrc_rb, sizeof(*eve), 0);
      if (eve != NULL) {
        eve->latency_ns = q2c;
        eve->type = IO_TYPE_Q2C;
        eve->dev = dev;
        eve->sector = sector;
        eve->bytes = nr_bytes;
        bpf_get_current_comm(&eve->comm, sizeof(eve->comm));
        bpf_ringbuf_submit(eve, 0);
      } // end if(eve)
    }

    // 清理
    bpf_map_delete_elem(&hebpf_iotrc_stm, &key);
  } // end if(time)

#if defined(THIS_ID)
  CHAIN_NEXT(ctx, Block_Rq_Complete, 0);
#endif
  return 0;
}

char LICENSE[] SEC("license") = "GPL";
