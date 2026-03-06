#pragma once

// clang-format off
#include <functional>
#include <memory>
#include <unordered_map>
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
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

private:
  std::string makeKey(std::string_view name,
                      const std::map<std::string, std::string> &labels) const;

  std::unique_ptr<prometheus::Exposer> exposer_;
  std::shared_ptr<prometheus::Registry> registry_;
  std::mutex mutex_;

  // 在哈希表里面只保存 prometheus::Counter 对象的引用

  std::unordered_map<std::string, std::reference_wrapper<prometheus::Counter>> counters_;
};

} // namespace monitor
} // namespace hebpf
