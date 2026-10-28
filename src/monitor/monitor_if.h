#pragma once

// clang-format off
#include <map>
#include <string>
#include <string_view>
#include <vector>
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
  virtual void observeHistogram(std::string_view name,
                                const std::map<std::string, std::string> &labels, double value,
                                const std::vector<double> &buckets) = 0;
  virtual void setGauge(std::string_view name, const std::map<std::string, std::string> &labels,
                        double value) = 0;
};

} // namespace monitor
} // namespace hebpf
