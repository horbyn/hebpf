#pragma once

// clang-format off
#include <string_view>
#include "src/log/logger.h"
#include "src/callback/callback.h"
#include "src/ebpf/ringbuffer_manager.h"
#include "klog.bpf.h"
// clang-format on

namespace hebpf {
namespace services {
namespace klog {

class Klog : public log::Loggable<log::Id::ebpf> {
public:
  explicit Klog(std::weak_ptr<io::IoIf> io_ctx, std::string_view pin_path = PATH_PIN,
                size_t map_size = KLOG_BUFFER_SIZE, std::string_view map_name = KLOG_BUFFER_NAME);

  Klog(const Klog &) = delete;
  Klog &operator=(const Klog &) = delete;

  Klog(Klog &&) noexcept = default;
  Klog &operator=(Klog &&) noexcept = default;

  void unpin();

private:
  int logEvent(void *data, size_t size);

  std::unique_ptr<ebpf::RingbufferManager> mgr_;
  std::unique_ptr<ebpf::Ringbuffer> ringbuf_;
};

} // namespace klog
} // namespace services
} // namespace hebpf
