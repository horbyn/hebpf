#pragma once

namespace hebpf {

constexpr int FD_INVALID = -1;

class FdIf {
public:
  virtual ~FdIf() = default;

  virtual int fd() const noexcept = 0;
};

} // namespace hebpf
