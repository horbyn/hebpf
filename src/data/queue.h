#pragma once

// clang-format off
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>
#include "queue_if.h"
// clang-format on

namespace hebpf {

template <typename T>
class Queue : public QueueIf<T> {
public:
  explicit Queue();
  explicit Queue(size_t capacity);

  Queue(const Queue &) = delete;
  Queue &operator=(const Queue &) = delete;

  // TODO: 放开移动
  Queue(Queue &&) = delete;
  Queue &operator=(Queue &&) = delete;

  void push(const T &value) override;
  void push(T &&value) override;
  std::optional<T> pop() override;
  std::optional<T> tryPop() override;
  bool empty() const override;
  void start() override;
  void stop() override;
  bool full() const override;

private:
  mutable std::mutex mutex_;
  std::queue<T> queue_;
  size_t capacity_;
  std::condition_variable cond_not_full_;
  std::condition_variable cond_not_empty_;
  std::atomic<bool> stopped_{false};
};

} // namespace hebpf

#include "queue.tpp"
