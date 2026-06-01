#pragma once

// clang-format off
#include "subscriber_if.h"
#include "src/daemon/configs.hpp"
// clang-format on

namespace hebpf {
namespace subscribe {

class YamlSubscriberIf : public SubscriberIf {
public:
  virtual ~YamlSubscriberIf() = default;
  virtual void update(const daemon::Configs &config) = 0;
};

} // namespace subscribe
} // namespace hebpf
