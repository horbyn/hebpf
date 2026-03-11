#pragma once

// clang-format off
#include "src/subscribe/subscriber_if.h"
// clang-format on

namespace hebpf {
namespace monitor {

class MonitorSubscriberIf : public subscribe::SubscriberIf {
public:
  virtual ~MonitorSubscriberIf() = default;
};

} // namespace monitor
} // namespace hebpf
