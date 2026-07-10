#pragma once

// clang-format off
#include <string_view>
// clang-format on

namespace hebpf {
namespace debug_server {

class DebugServerIf {
public:
  ~DebugServerIf() = default;

  virtual bool start(std::string_view address, uint16_t port) = 0;
  virtual void stop() = 0;
};

} // namespace debug_server
} // namespace hebpf
