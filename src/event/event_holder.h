#pragma once

// clang-format off
#include <memory>
#include "event_bus_if.hpp"
// clang-format on

namespace hebpf {
namespace event {

/**
 * @brief 混入基类：为派生类提供 event bus 的持有与设置能力
 * @note 非虚设计；如果某个类需要额外逻辑，可在派生类中隐藏同名函数
 */
class EventBusHolder {
public:
  explicit EventBusHolder(std::weak_ptr<EventBusIf> event_bus);
  virtual ~EventBusHolder() = default;

protected:
  std::weak_ptr<EventBusIf> bus_;
};

} // namespace event
} // namespace hebpf
