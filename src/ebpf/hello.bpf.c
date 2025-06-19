#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int handle_execve(struct trace_event_raw_sys_enter *ctx)
{
  char msg[] = "Hello from eBPF! Process executed: ";
  bpf_printk("%s", msg);
  return 0;
}

char _license[] SEC("license") = "GPL";
