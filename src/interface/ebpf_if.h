#pragma once

// clang-format off
#include <memory>
// clang-format on

namespace hebpf {

class EbpfIf {
public:
  virtual ~EbpfIf() = default;

  virtual void open() = 0;
  virtual void load() = 0;
  virtual void attach() = 0;
  virtual void detach() = 0;
  virtual void destroy() = 0;
};

} // namespace hebpf

std::unique_ptr<hebpf::EbpfIf> create_ebpf();

#define SERVICE_CREATE "create_service"
