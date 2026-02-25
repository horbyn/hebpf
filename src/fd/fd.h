#pragma once

// clang-format off
#include "fd_if.h"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {

class Fd : public FdIf, public log::Loggable<log::Id::fd> {
public:
  explicit Fd() = default;
  explicit Fd(int fd);
  ~Fd();

  Fd(const Fd &) = delete;
  Fd &operator=(const Fd &) = delete;

  Fd(Fd &&other) noexcept;
  Fd &operator=(Fd &&other) noexcept;

  int fd() const noexcept override;

protected:
  int fd_ = FD_INVALID;
};

} // namespace hebpf
