#pragma once

// clang-format off
#include <bpf/libbpf.h>
#include <functional>
#include <memory>
#include <mutex>
#include "src/callback/callback.h"
#include "src/fd/fd_if.h"
#include "src/io/io_if.h"
#include "src/log/logger.h"
#include "ringbuffer_if.h"
// clang-format on

namespace hebpf {
namespace ebpf {

class Ringbuffer : public RingbufferIf, public log::Loggable<log::Id::ebpf> {
public:
  using RingbufferCb = common::Callback<int(void *, size_t)>;

  explicit Ringbuffer(std::unique_ptr<FdIf> fd, std::weak_ptr<io::IoIf> io_ctx,
                      RingbufferCb callback = RingbufferCb{});

  Ringbuffer(const Ringbuffer &) = delete;
  Ringbuffer &operator=(const Ringbuffer &) = delete;

  Ringbuffer(Ringbuffer &&other) noexcept;
  Ringbuffer &operator=(Ringbuffer &&other) noexcept;

  void init() override;

private:
  void consume() const;
  struct RingBufferDeleter {
    void operator()(struct ring_buffer *rb) const noexcept {
      if (rb) {
        ring_buffer__free(rb);
      }
    }
  };

  std::unique_ptr<FdIf> fd_;
  std::unique_ptr<struct ring_buffer, RingBufferDeleter> ringbuff_;
  RingbufferCb callback_;
  std::shared_ptr<void> watcher_;
  std::weak_ptr<io::IoIf> io_ctx_;
  mutable std::mutex mutex_;
};

} // namespace ebpf
} // namespace hebpf
