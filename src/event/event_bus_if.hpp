#pragma once

// clang-format off
#include <typeindex>
#include "src/callback/callback.h"
#include "event.h"
// clang-format on

namespace hebpf {
namespace event {

using EventHandler = common::Callback<void(const EventIf &)>;

class EventBusIf {
public:
  virtual ~EventBusIf() = default;

  template <typename T>
  void subscribe(EventHandler handler);

  virtual void subscribeImpl(std::type_index type, EventHandler handler) = 0;
  virtual void publish(const EventIf &event) = 0;
};

/**
 * @brief 订阅事件
 *
 * @param handler 事件处理函数
 */
template <typename T>
inline void EventBusIf::subscribe(EventHandler handler) {
  subscribeImpl(std::type_index(typeid(T)), std::move(handler));
}

} // namespace event
} // namespace hebpf
