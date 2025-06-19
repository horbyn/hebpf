#include "hello.skel.h"
#include <stdio.h>
#include <unistd.h>

int main()
{
  struct hello_bpf *skel = hello_bpf__open();
  if (!skel)
  {
    perror("Failed to open BPF skeleton");
    return 1;
  }

  if (hello_bpf__load(skel))
  { // 加载并验证 BPF 程序
    fprintf(stderr, "Failed to load BPF skeleton\n");
    hello_bpf__destroy(skel);
    return 1;
  }

  if (hello_bpf__attach(skel))
  { // 附加到挂载点
    fprintf(stderr, "Failed to attach BPF program\n");
    hello_bpf__destroy(skel);
    return 1;
  }

  printf("eBPF program running! Run `sudo cat /sys/kernel/debug/tracing/trace_pipe` to view logs.\n");
  while (1)
    sleep(1); // 保持程序运行

  hello_bpf__destroy(skel);
  return 0;
}
