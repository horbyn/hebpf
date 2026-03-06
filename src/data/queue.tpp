#pragma once

// clang-format off
#include "queue.h"
// clang-format on

namespace hebpf {

template <typename T>
Queue<T>::Queue() : capacity_{MAX_QUEUE_SIZE} {}

template <typename T>
Queue<T>::Queue(size_t capacity) : Queue{} {
  capacity_ = capacity;
}

/**
 * @brief 元素入队
 *
 * @tparam T 元素类型
 * @param value 元素
 */
template <typename T>
void Queue<T>::push(const T &value) {
  std::unique_lock<std::mutex> lock{mutex_};
  cond_not_full_.wait(lock, [this] { return queue_.size() < capacity_ || stopped_.load(); });
  if (stopped_.load()) {
    return;
  }
  queue_.push(value);
  cond_not_empty_.notify_one();
}

/**
 * @brief 元素入队
 *
 * @tparam T 元素类型
 * @param value 元素
 */
template <typename T>
void Queue<T>::push(T &&value) {
  std::unique_lock<std::mutex> lock{mutex_};
  cond_not_full_.wait(lock, [this] { return queue_.size() < capacity_ || stopped_.load(); });
  if (stopped_.load()) {
    return;
  }
  queue_.push(std::move(value));
  cond_not_empty_.notify_one();
}

/**
 * @brief 元素出队
 *
 * @tparam T 元素类型
 * @return std::optional<T> 元素
 */
template <typename T>
std::optional<T> Queue<T>::pop() {
  {
    std::unique_lock<std::mutex> lock{mutex_};
    cond_not_empty_.wait(lock, [this] { return !queue_.empty() || stopped_.load(); });
  }
  return tryPop();
}

/**
 * @brief 判断队头元素
 *
 * @tparam T 元素类型
 * @return std::optional<T> 队头有元素可以出队，没有返回空
 */
template <typename T>
std::optional<T> Queue<T>::tryPop() {
  std::lock_guard<std::mutex> lock{mutex_};
  if (stopped_.load() || (!stopped_.load() && queue_.empty())) {
    return std::nullopt;
  }
  T value = std::move(queue_.front());
  queue_.pop();
  cond_not_full_.notify_one();
  return value;
}

/**
 * @brief 判断队列是否为空
 *
 * @tparam T 元素类型
 * @return true 空
 * @return false 非空
 */
template <typename T>
bool Queue<T>::empty() const {
  std::lock_guard<std::mutex> lock{mutex_};
  return queue_.empty();
}

/**
 * @brief 停止队列
 */
template <typename T>
void Queue<T>::stop() {
  stopped_.store(true);
  cond_not_empty_.notify_all();
  cond_not_full_.notify_all();
}

} // namespace hebpf
