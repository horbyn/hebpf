// clang-format off
#include "event_bus.h"
#include "src/common/stack_frame.h"
// clang-format on

namespace hebpf {
namespace event {

/**
 * @brief 订阅事件
 *
 * @param type    事件类型索引，对应 subscribe<T> 中的 T
 * @param handler 事件处理处理函数
 */
void EventBus::subscribeImpl(std::type_index type, EventHandler handler) {
  std::lock_guard<std::mutex> lock{mutex_};

  // COW: 更新 handlers_ 时拷贝一份，避免影响在 publish() 的其他线程（此时其他线程读取的是旧副本）
  auto iter = handlers_.find(type);
  if (iter == handlers_.end()) {
    // 首次订阅：直接创建
    auto new_list = std::make_shared<std::vector<EventHandler>>();
    new_list->push_back(std::move(handler));
    handlers_.emplace(type, std::move(new_list));
  } else {
    // 已存在：写时复制，替换指针
    auto new_list = std::make_shared<std::vector<EventHandler>>(*iter->second);
    new_list->push_back(std::move(handler));
    iter->second = std::move(new_list);
  }
}

/**
 * @brief 发布事件
 *
 * @param event 事件对象
 */
void EventBus::publish(const EventIf &event) {
  LOG(debug, "[publish] looking for key: {}", typeid(event).name());

  HandlerVec handlers{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    auto iter = handlers_.find(typeid(event));
    if (iter == handlers_.end()) {
      return;
    }
    handlers = iter->second; // 仅拷贝指针
  }

  for (auto &handle : *handlers) {
    handle(event);
  }
}

/**
 * @brief 更新调试接口配置
 *
 * @param out 配置文件
 */
void EventBus::update(nlohmann::json &out) {
  auto obj = nlohmann::json::object();

  decltype(handlers_) handlers_copy{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    handlers_copy = handlers_;
  }

  // TODO: 实现一个独立的对象
  for (const auto &[type, vec] : handlers_copy) {
    auto key = demangle(type.name());
    nlohmann::json names = nlohmann::json::array();

    if (vec != nullptr) {
      for (const auto &handler : *vec) {
        names.push_back(handler.name());
      }
    }
    obj[std::move(key)] = std::move(names);
  } // end for()

  out[JKEY_DBGSERVER_EVENTBUS] = obj;
}

} // namespace event
} // namespace hebpf
