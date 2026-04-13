#pragma once

// clang-format off
#include <memory>
#include <string>
#include <string_view>
#include "ringbuffer.h"
#include "src/log/logger.h"
#include "src/services/klog/klog.bpf.h"
// clang-format on

namespace hebpf {
namespace ebpf {

class RingbufferManager : public log::Loggable<log::Id::ebpf> {
public:
  explicit RingbufferManager(std::string_view pin_path);

  RingbufferManager(const RingbufferManager &) = delete;
  RingbufferManager &operator=(const RingbufferManager &) = delete;

  RingbufferManager(RingbufferManager &&other) noexcept = default;
  RingbufferManager &operator=(RingbufferManager &&other) noexcept = default;

  std::unique_ptr<Ringbuffer> getPinningRingbuffer(std::weak_ptr<io::IoIf> io_ctx,
                                                   Ringbuffer::RingbufferCb callback,
                                                   size_t map_size, std::string_view map_name);
  void unpin();

private:
  std::string pin_path_;
};

} // namespace ebpf
} // namespace hebpf
