#pragma once

// clang-format off
#include <memory>
#include "subscriber_if.h"
// clang-format on

namespace hebpf {
namespace subscribe {

class PublisherIf {
public:
  virtual ~PublisherIf() = default;
  virtual void attach(std::shared_ptr<SubscriberIf> observer) = 0;
  virtual void detach(std::shared_ptr<SubscriberIf> observer) = 0;
  virtual void notify() = 0;
};

} // namespace subscribe
} // namespace hebpf
