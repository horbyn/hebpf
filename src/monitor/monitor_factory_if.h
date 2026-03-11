#pragma once

// clang-format off
#include <memory>
#include <string_view>
#include "monitor_if.h"
// clang-format on

namespace hebpf {
namespace monitor {

class MonitorFactoryIf {
public:
  virtual ~MonitorFactoryIf() = default;
  virtual std::shared_ptr<MonitorIf> create(std::string_view bind_address) = 0;
};

} // namespace monitor
} // namespace hebpf
