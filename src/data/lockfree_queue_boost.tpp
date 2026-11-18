#pragma once

// clang-format off
#include "lockfree_queue_boost.h"
// clang-format on

namespace hebpf {

template <typename T>
LockfreeQueueBoost<T>::LockfreeQueueBoost(size_t capacity) : queue_(capacity) {}

/**
 * @brief 元素入队
 *
 * @tparam T 元素类型
 * @param value 元素
 */
template <typename T>
void LockfreeQueueBoost<T>::push(const T &value) {
  while (true) {
    if (stopped_.load(std::memory_order_relaxed)) {
      return;
    }
    if (queue_.push(value)) {
      return;
    }
    std::this_thread::yield();
  }
}

/**
 * @brief 元素入队
 *
 * @tparam T 元素类型
 * @param value 元素
 */
template <typename T>
void LockfreeQueueBoost<T>::push(T &&value) {
  while (true) {
    if (stopped_.load(std::memory_order_relaxed)) {
      return;
    }
    if (queue_.push(std::move(value))) {
      return;
    }
    std::this_thread::yield();
  }
}

/**
 * @brief 元素出队
 *
 * @tparam T 元素类型
 * @return std::optional<T> 元素
 */
template <typename T>
std::optional<T> LockfreeQueueBoost<T>::pop() {
  T value;
  while (true) {
    if (stopped_.load(std::memory_order_relaxed) && queue_.empty()) {
      return std::nullopt;
    }
    if (queue_.pop(value)) {
      return value;
    }
    std::this_thread::yield();
  }
}

/**
 * @brief 判断队头元素
 *
 * @tparam T 元素类型
 * @return std::optional<T> 队头有元素可以出队，没有返回空
 */
template <typename T>
std::optional<T> LockfreeQueueBoost<T>::tryPop() {
  T value;
  if (queue_.pop(value)) {
    return value;
  }
  return std::nullopt;
}

/**
 * @brief 判断队列是否为空
 *
 * @tparam T 元素类型
 * @return true 空
 * @return false 非空
 */
template <typename T>
bool LockfreeQueueBoost<T>::empty() const {
  return queue_.empty();
}

/**
 * @brief 启动队列
 *
 */
template <typename T>
void LockfreeQueueBoost<T>::start() {
  stopped_.store(false, std::memory_order_relaxed);
}

/**
 * @brief 停止队列
 */
template <typename T>
void LockfreeQueueBoost<T>::stop() {
  stopped_.store(true, std::memory_order_relaxed);
}

/**
 * @brief 判断队列满
 *
 * @return true 是的
 * @return false 不是
 */
template <typename T>
bool LockfreeQueueBoost<T>::full() const {
  // boost::lockfree::queue 不会满，push 失败时返回 false
  return false;
}

/**
 * @brief 批量取出所有数据，并清空队列
 *
 * @note 依赖于 boost::lockfree::queue 原生的 pop 接口，非阻塞提取
 *
 * @return std::vector<T> 包含所有提取出的元素
 */
template <typename T>
std::vector<T> LockfreeQueueBoost<T>::drain() {
  std::vector<T> out{};

  // 因为 boost::lockfree::queue 可能随时有新数据插入，
  // 这里只能逐个弹出，直到遇到空
  T value{};
  while (queue_.pop(value)) {
    out.push_back(std::move(value));
  }

  return out;
}

} // namespace hebpf
