#pragma once

// clang-format off
#include "lockfree_queue_boost.h"
// clang-format on

namespace hebpf {

template <typename T>
LockfreeQueueBoost<T>::LockfreeQueueBoost(size_t capacity) : queue_(capacity) {}

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

template <typename T>
std::optional<T> LockfreeQueueBoost<T>::tryPop() {
  T value;
  if (queue_.pop(value)) {
    return value;
  }
  return std::nullopt;
}

template <typename T>
bool LockfreeQueueBoost<T>::empty() const {
  return queue_.empty();
}

template <typename T>
void LockfreeQueueBoost<T>::start() {
  stopped_.store(false, std::memory_order_relaxed);
}

template <typename T>
void LockfreeQueueBoost<T>::stop() {
  stopped_.store(true, std::memory_order_relaxed);
}

template <typename T>
bool LockfreeQueueBoost<T>::full() const {
  // boost::lockfree::queue 不会满，push 失败时返回 false
  return false;
}

} // namespace hebpf
