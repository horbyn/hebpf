#pragma once

// clang-format off
#include "src/daemon/configs.hpp"
// clang-format on

namespace hebpf {
namespace subscribe {

class SubscriberIf {
public:
  virtual ~SubscriberIf() = default;
  virtual void update(const daemon::Configs &config) = 0;
};

} // namespace subscribe
} // namespace hebpf
