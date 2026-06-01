#pragma once

// clang-format off
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {
namespace subscribe {

class SubscriberIf {
public:
  virtual ~SubscriberIf() = default;
};

} // namespace subscribe
} // namespace hebpf
