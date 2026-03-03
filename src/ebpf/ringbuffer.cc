// clang-format off
#include "ringbuffer.h"
#include "src/common/assert.h"
#include "src/common/common.h"
#include "src/common/exception.h"
// clang-format on

namespace hebpf {
namespace ebpf {

Ringbuffer::Ringbuffer(std::unique_ptr<FdIf> fd, std::weak_ptr<io::IoIf> io_ctx,
                       RingbufferCb callback)
    : fd_{std::move(fd)}, callback_{std::move(callback)}, io_ctx_{io_ctx} {}

Ringbuffer::Ringbuffer(Ringbuffer &&other) noexcept
    : fd_{std::move(other.fd_)}, ringbuff_{std::move(other.ringbuff_)},
      callback_{std::move(other.callback_)} {
  other.fd_ = nullptr;
  other.ringbuff_ = nullptr;
}

Ringbuffer &Ringbuffer::operator=(Ringbuffer &&other) noexcept {
  if (this != &other) {
    fd_ = std::move(other.fd_);
    ringbuff_ = std::move(other.ringbuff_);
    callback_ = std::move(other.callback_);
    other.fd_ = nullptr;
    other.ringbuff_ = nullptr;
  }
  return *this;
}

/**
 * @brief 取出内核事件
 *
 */
void Ringbuffer::consume() const noexcept {
  if (ringbuff_ != nullptr) {
    ring_buffer__consume(ringbuff_.get());
  }
}

/**
 * @brief 初始化
 *
 */
void Ringbuffer::init() {
  if (fd_ == nullptr || fd_->fd() == FD_INVALID) {
    throw EXCEPT("MUST provide a eBPF program fd");
  }

  auto raw_cb = [](void *ctx, void *data, size_t data_sz) -> int {
    auto *self = static_cast<Ringbuffer *>(ctx);
    if (self != nullptr && self->callback_) {
      return self->callback_(data, data_sz);
    }
    return 0;
  };
  ringbuff_.reset(ring_buffer__new(fd_->fd(), raw_cb, this, nullptr));
  if (ringbuff_ == nullptr) {
    throw EXCEPT("ring_buffer__new() failed", true);
  }

  auto io_ctx_val = io_ctx_.lock();
  if (io_ctx_val == nullptr) {
    throw EXCEPT("MUST provide a io context");
  }
  watcher_ = io_ctx_val->addReadCb(fd_->fd(), io::IoCb{FUNCTION_LINE, [this]() { consume(); }});
}

} // namespace ebpf
} // namespace hebpf
