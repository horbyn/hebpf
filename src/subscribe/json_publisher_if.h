#pragma once

// clang-format off
#include "publisher_if.h"
#include "json_subscriber_if.h"
// clang-format on

namespace hebpf {
namespace subscribe {

class JsonPublisherIf : public PublisherIf {
public:
  virtual ~JsonPublisherIf() = default;
  virtual void attach(std::shared_ptr<JsonSubscriberIf> observer) = 0;
  virtual void detach(std::shared_ptr<JsonSubscriberIf> observer) = 0;
  virtual nlohmann::json notify(void) = 0;
};

} // namespace subscribe
} // namespace hebpf
