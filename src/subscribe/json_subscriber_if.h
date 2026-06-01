#pragma once

// clang-format off
#include "nlohmann/json.hpp"
#include "subscriber_if.h"
// clang-format on

namespace hebpf {
namespace subscribe {

class JsonSubscriberIf : public SubscriberIf {
public:
  virtual ~JsonSubscriberIf() = default;
  virtual void update(nlohmann::json &out) = 0;
};

} // namespace subscribe
} // namespace hebpf
