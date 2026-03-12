#pragma once

// clang-format off
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "monitor_factory_if.h"
// clang-format on

namespace hebpf {
namespace monitor {

class MonitorFactory : public MonitorFactoryIf {
public:
  std::shared_ptr<MonitorIf> create(std::string_view bind_address) override;

private:
  std::mutex mutex_;
  std::unordered_map<std::string, std::weak_ptr<MonitorIf>> cache_;
};

} // namespace monitor
} // namespace hebpf
