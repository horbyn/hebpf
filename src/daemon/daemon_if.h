#pragma once

// clang-format off
#include <string_view>
#include "configs.hpp"
// clang-format on

namespace hebpf {
namespace daemon {

class DaemonIf {
public:
  virtual ~DaemonIf() = default;

  virtual void run() = 0;
  virtual void stop() = 0;
};

} // namespace daemon
} // namespace hebpf
