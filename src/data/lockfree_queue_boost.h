#pragma once

// clang-format off
#include <atomic>
#include "boost/lockfree/queue.hpp"
#include "queue_if.h"
// clang-format on

namespace hebpf {

/**
 * @brief 基于 boost::lockfree::queue 的 MPSC 队列封装
 *
 * @tparam T 必须可拷贝或可移动
 */
template <typename T>
class LockfreeQueueBoost : public QueueIf<T> {
  static_assert(std::is_trivially_copyable_v<T> || std::is_pointer_v<T>,
                "T must be trivially copyable or a pointer type for boost::lockfree::queue");

public:
  explicit LockfreeQueueBoost(size_t capacity);

  void push(const T &value) override;
  void push(T &&value) override;
  std::optional<T> pop() override;
  std::optional<T> tryPop() override;
  bool empty() const override;
  void start() override;
  void stop() override;
  bool full() const override;

private:
  boost::lockfree::queue<T> queue_;
  std::atomic<bool> stopped_{false};
};

} // namespace hebpf

#include "lockfree_queue_boost.tpp"
