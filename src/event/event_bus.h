#pragma once

// clang-format off
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "src/log/logger.h"
#include "src/subscribe/json_subscriber_if.h"
#include "event_bus_if.hpp"
// clang-format on

namespace hebpf {
namespace event {

constexpr std::string_view JKEY_DBGSERVER_EVENTBUS{"EventBus"};

class EventBus : public EventBusIf,
                 public subscribe::JsonSubscriberIf,
                 public log::Loggable<log::Id::event> {
public:
  void subscribeImpl(std::type_index type, EventHandler handler) override;
  void publish(const EventIf &event) override;
  void update(nlohmann::json &out) override;

private:
  using HandlerVec = std::shared_ptr<std::vector<EventHandler>>;

  std::mutex mutex_;
  std::unordered_map<std::type_index, HandlerVec> handlers_;
};

} // namespace event
} // namespace hebpf
