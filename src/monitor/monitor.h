#pragma once

// clang-format off
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/gauge.h"
#include "prometheus/histogram.h"
#include "prometheus/registry.h"
#include "src/log/logger.h"
#include "monitor_if.h"
// clang-format on

namespace hebpf {
namespace monitor {

class Monitor : public MonitorIf, public log::Loggable<log::Id::prometheus> {
public:
  explicit Monitor(std::string_view bind_address);

  void incrementCounter(std::string_view name, const std::map<std::string, std::string> &labels,
                        double value = INC_VALUE) override;

  void observeHistogram(std::string_view name, const std::map<std::string, std::string> &labels,
                        double value, const std::vector<double> &buckets) override;

  void setGauge(std::string_view name, const std::map<std::string, std::string> &labels,
                double value) override;

private:
  std::string makeKey(std::string_view name,
                      const std::map<std::string, std::string> &labels) const;

  std::unique_ptr<prometheus::Exposer> exposer_;
  std::shared_ptr<prometheus::Registry> registry_;
  std::mutex mutex_;

  // 缓存已创建指标对象的引用，避免每次 BuildXXX().Register() 的开销
  std::unordered_map<std::string, std::reference_wrapper<prometheus::Counter>> counters_;
  std::unordered_map<std::string, std::reference_wrapper<prometheus::Histogram>> histograms_;
  std::unordered_map<std::string, std::reference_wrapper<prometheus::Gauge>> gauges_;
};

} // namespace monitor
} // namespace hebpf
