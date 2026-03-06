#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <map>
// clang-format on

namespace hebpf {
namespace monitor {

constexpr double INC_VALUE{1.0};

class MonitorIf {
public:
  virtual ~MonitorIf() = default;
  virtual void incrementCounter(std::string_view name,
                                const std::map<std::string, std::string> &labels,
                                double value = INC_VALUE) = 0;
};

} // namespace monitor
} // namespace hebpf
