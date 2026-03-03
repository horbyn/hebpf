# examples

这个目录中保存了 hebpf 给出的 eBPF 示例程序：
- `hello`：hello world 级别的示例程序，绑定在 `exec*()` 系统调用的 hook，任何需要切换执行流的命令都可以触发，如 `ls`、`cat` 等（`cd` 除外）。由于是最入门的程序，所以仅仅使用了 `/sys/kernel/debug/tracing/trace_pipe` 来作为内核态的输出
- `usermode`：同样绑定在 `exec*()` 系统调用的 hook，和前一个程序区别是使用了 eBPF map 来作为内核态、用户态通信的机制，只要 eBPF map 有内核数据到达，用户态程序就会将数据打印到日志
