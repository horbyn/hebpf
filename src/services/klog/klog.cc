// clang-format off
#include "klog.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include "src/common/common.h"
// clang-format on

namespace hebpf {
namespace services {
namespace klog {

Klog::Klog(std::weak_ptr<io::IoIf> io_ctx, std::string_view pin_path, size_t map_size,
           std::string_view map_name)
    : mgr_(std::make_unique<ebpf::RingbufferManager>(pin_path)), ringbuf_{} {

  ringbuf_ = mgr_->getPinningRingbuffer(
      io_ctx,
      ebpf::Ringbuffer::RingbufferCb{
          FUNCTION_LINE, [this](void *data, size_t len) { return logEvent(data, len); }},
      map_size, map_name);
}

/**
 * @brief 删除文件系统中的 pinned 文件
 *
 */
void Klog::unpin() {
  if (mgr_ != nullptr) {
    mgr_->unpin();
  }
}

/**
 * @brief 内核日志回报
 *
 * @param data 内核日志
 * @param size 日志对象大小
 * @return int 成功返回 0，出错返回非 0
 */
int Klog::logEvent(void *data, size_t size) {
  if (size < sizeof(struct log_event)) {
    LOG(error, "invalid event size: {} for log_event object size: {}", size,
        sizeof(struct log_event));
    return -1;
  }

  const struct log_event *event = static_cast<const struct log_event *>(data);
  if (size < offsetof(struct log_event, msg)) {
    LOG(error, "log_event size: {} is too small", size);
    return -1;
  }

  auto klevel = event->level;
  if (static_cast<uint32_t>(klevel) < static_cast<uint32_t>(getLevel())) {
    return 0;
  }

  char formatted_msg[1024]{};
  int pos = 0;
  const char *fmt = event->msg;
  int arg_idx = 0;

  while (*fmt && pos < (int)sizeof(formatted_msg) - 1) {
    if (*fmt != '%') {
      formatted_msg[pos++] = *fmt++;
      continue;
    }

    ++fmt; // 跳过 %
    if (*fmt == '%') {
      formatted_msg[pos++] = '%';
      ++fmt;
      continue;
    }

    char spec = *fmt;
    // 支持原生格式化字符 d, u, x, X, p
    // 和 hebpf 扩展字符 P, A
    if (spec == 'd' || spec == 'u' || spec == 'x' || spec == 'X' || spec == 'p' || spec == 'P' ||
        spec == 'A') {
      if (arg_idx >= MAX_ARGS_CNT) {
        pos += snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "<missing>");
        ++fmt;
        continue;
      }
      uint64_t val = event->args[arg_idx++];

      int len = 0;
      switch (spec) {
      case 'd':
        len = snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "%lld", (long long)val);
        break;
      case 'u':
        len = snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "%llu",
                       (unsigned long long)val);
        break;
      case 'x':
        len = snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "%llx",
                       (unsigned long long)val);
        break;
      case 'X':
        len = snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "%llX",
                       (unsigned long long)val);
        break;
      case 'p':
        len = snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "%p",
                       (void *)(uintptr_t)val);
        break;
      case 'P': // 输出协议
        switch (val) {
        case IPPROTO_TCP:
          len = snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "TCP");
          break;
        case IPPROTO_UDP:
          len = snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "UDP");
          break;
        case IPPROTO_ICMP:
          len = snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "ICMP");
          break;
        default:
          len = snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "%ld", val);
          break;
        }
        break;
      case 'A': // 输出 IPv4 地址
        len = snprintf(formatted_msg + pos, sizeof(formatted_msg) - pos, "%s",
                       inet_ntoa((struct in_addr){static_cast<in_addr_t>(val)}));
        break;
      default:
        len = 0;
        break;
      } // end switch()
      if (len > 0) {
        pos += len;
      }
      ++fmt;
    } else {
      formatted_msg[pos++] = '%';
      formatted_msg[pos++] = spec;
      ++fmt;
    }
  } // end while()
  formatted_msg[pos] = '\0';

  // TODO: 这个 switch-case 写得很丑，要优化

  switch (klevel) {
  case KLOG_LEVEL_TRACE:
    LOG(trace, "{}", formatted_msg);
    break;
  case KLOG_LEVEL_DEBUG:
    LOG(debug, "{}", formatted_msg);
    break;
  case KLOG_LEVEL_INFO:
    LOG(info, "{}", formatted_msg);
    break;
  case KLOG_LEVEL_WARN:
    LOG(warn, "{}", formatted_msg);
    break;
  case KLOG_LEVEL_ERROR:
    LOG(error, "{}", formatted_msg);
    break;
  case KLOG_LEVEL_CRITICAL:
    LOG(critical, "{}", formatted_msg);
    break;
  default:
    LOG(off, "{}", formatted_msg);
    break;
  }

  return 0;
}

} // namespace klog
} // namespace services
} // namespace hebpf
