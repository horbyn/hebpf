// clang-format off
#include "fd.h"
#include <unistd.h>
// clang-format on

namespace hebpf {

Fd::Fd(int fd) : fd_{fd} {}

Fd::~Fd() {
  if (fd_ != FD_INVALID) {
    close(fd_);
  }
}

Fd::Fd(Fd &&other) noexcept : fd_(other.fd_) { other.fd_ = FD_INVALID; }

Fd &Fd::operator=(Fd &&other) noexcept {
  if (this != &other) {
    if (fd_ != FD_INVALID)
      close(fd_);
    fd_ = other.fd_;
    other.fd_ = FD_INVALID;
  }
  return *this;
}

/**
 * @brief 获取 fd
 *
 * @return int fd
 */
int Fd::fd() const noexcept { return fd_; }

} // namespace hebpf
