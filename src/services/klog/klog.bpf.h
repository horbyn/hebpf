#pragma once

/*
 * 这个文件内核态和用户态都要引用，所以不能包含
 * （1）C/C++ 标准库，因为这种头文件只有用户态能用
 * （2）bpf 头文件，因为这种头文件只有内核态能用
 *
 * 最终这里只能使用基础数据类型
 */

#define MAX_ARGS_CNT 10
#define KLOG_BUFFER_SIZE (256 * 1024) // 256 KB
#define KLOG_BUFFER_NAME "hebpf_klog"
#define PATH_BPFFS "/sys/fs/bpf/"
#define PATH_PIN PATH_BPFFS KLOG_BUFFER_NAME

#ifdef __cplusplus
extern "C" {
#endif

struct log_event {
  uint16_t level;              // 日志等级
  uint64_t args[MAX_ARGS_CNT]; // 日志参数
  uint16_t msg_len;            // 格式化字符串长度
  char msg[];                  // 格式化字符串
};

#ifdef __cplusplus
}
#endif

// TODO: 对应日志对象的等级不应该手动设置，应该跟根据日志等级自动设置

#define KLOG_LEVEL_TRACE 0
#define KLOG_LEVEL_DEBUG 1
#define KLOG_LEVEL_INFO 2
#define KLOG_LEVEL_WARN 3
#define KLOG_LEVEL_ERROR 4
#define KLOG_LEVEL_CRITICAL 5
#define KLOG_LEVEL_OFF 6
