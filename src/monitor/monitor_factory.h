#pragma once

// clang-format off
#include "monitor_factory_if.h"
// clang-format on

namespace hebpf {
namespace monitor {

class MonitorFactory : public MonitorFactoryIf {
public:
  std::shared_ptr<MonitorIf> create(std::string_view bind_address) override;
};

} // namespace monitor
} // namespace hebpf
