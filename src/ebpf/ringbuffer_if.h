#pragma once

namespace hebpf {
namespace ebpf {

class RingbufferIf {
public:
  virtual ~RingbufferIf() = default;

  virtual void init() = 0;
};

} // namespace ebpf
} // namespace hebpf
