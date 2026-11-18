// clang-format off
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h> // 提供 BPF_PROG 宏
#include "src/ebpf/ebpf_chain.h"
// clang-format on

SEC("?raw_tp/block_bio_queue")
int BPF_PROG(hebpf_scheduler_block_bio_queue, struct bio *bio) {
#ifdef THIS_ID
  CHAIN_NEXT(ctx, Block_Bio_Queue, 0);
#endif
  return 0;
}

SEC("?raw_tp/block_io_start")
int BPF_PROG(hebpf_scheduler_block_io_start, struct request *rq) {
#ifdef THIS_ID
  CHAIN_NEXT(ctx, Block_Io_Start, 0);
#endif
  return 0;
}

SEC("?raw_tp/block_rq_issue")
int BPF_PROG(hebpf_scheduler_block_rq_issue, struct request *rq) {
#ifdef THIS_ID
  CHAIN_NEXT(ctx, Block_Rq_Issue, 0);
#endif
  return 0;
}

SEC("?raw_tp/block_rq_complete")
int BPF_PROG(hebpf_scheduler_block_rq_complete, struct request *rq, blk_status_t error,
             unsigned int nr_bytes) {
#ifdef THIS_ID
  CHAIN_NEXT(ctx, Block_Rq_Complete, 0);
#endif
  return 0;
}

char _license[] SEC("license") = "GPL";
