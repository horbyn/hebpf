#pragma once

// clang-format off
#include <atomic>
#include <type_traits>
#include "queue_if.h"
// clang-format on

namespace hebpf {

/**
 * @brief 有界无锁队列（基于序列号环形缓冲区）
 * 容量固定，所有内存预先分配
 *
 * @tparam T 元素类型，必须满足：
 *           - 默认可构造（default constructible）
 *           - 可拷贝赋值或可移动赋值
 */
template <typename T>
class LockfreeQueue : public QueueIf<T> {
  static_assert(std::is_default_constructible_v<T>,
                "T must be default constructible for LockfreeQueue");

public:
  explicit LockfreeQueue(size_t capacity);
  ~LockfreeQueue() override = default;

  LockfreeQueue(const LockfreeQueue &) = delete;
  LockfreeQueue &operator=(const LockfreeQueue &) = delete;

  void push(const T &value) override;
  void push(T &&value) override;
  std::optional<T> pop() override;
  std::optional<T> tryPop() override;
  bool empty() const override;
  void start() override;
  void stop() override;
  bool full() const override;

private:
  struct Slot {
    std::atomic<size_t> sequence;
    T data;
  };

  alignas(64) std::atomic<size_t> enqueue_pos_{0};
  alignas(64) std::atomic<size_t> dequeue_pos_{0};
  alignas(64) std::atomic<bool> stopped_{false};

  const size_t capacity_;
  std::unique_ptr<Slot[]> buffer_;

  template <typename U>
  void assignToSlot(Slot &slot, U &&value);
};

} // namespace hebpf

#include "lockfree_queue.tpp"
