#pragma once

// clang-format off
#include "publisher_if.h"
#include "yaml_subscriber_if.h"
// clang-format on

namespace hebpf {
namespace subscribe {

class YamlPublisherIf : public PublisherIf {
public:
  virtual ~YamlPublisherIf() = default;
  virtual void attach(std::shared_ptr<YamlSubscriberIf> observer) = 0;
  virtual void detach(std::shared_ptr<YamlSubscriberIf> observer) = 0;
  virtual void notify() = 0;
};

} // namespace subscribe
} // namespace hebpf
