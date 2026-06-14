#pragma once

// clang-format off
#include "lockfree_queue.h"
// clang-format on

namespace hebpf {

template <typename T>
LockfreeQueue<T>::LockfreeQueue(size_t capacity)
    : capacity_(capacity), buffer_(std::make_unique<Slot[]>(capacity)) {
  // 初始化每个槽位的序列号（等于其索引）
  for (size_t i = 0; i < capacity_; ++i) {
    buffer_[i].sequence.store(i, std::memory_order_relaxed);
  }
}

/**
 * @brief 元素入队
 *
 * @tparam T 元素类型
 * @param value 元素
 */
template <typename T>
void LockfreeQueue<T>::push(const T &value) {
  size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
  while (true) {
    if (stopped_.load(std::memory_order_relaxed)) {
      return;
    }

    Slot &slot = buffer_[pos % capacity_];
    size_t seq = slot.sequence.load(std::memory_order_acquire);
    intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

    if (diff == 0) {
      // 槽位可用，尝试原子地递增 enqueue_pos_
      // 成功后，该生产者独占这个位置，其他生产者会竞争下一个位置
      if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
        // 成功保留位置，写入数据并更新序列号
        assignToSlot(slot, value);
        slot.sequence.store(pos + 1, std::memory_order_release);
        return;
      }

    } else if (diff < 0) {
      // 队列已满
      // 让出 CPU 等待消费者
      std::this_thread::yield();
      pos = enqueue_pos_.load(std::memory_order_relaxed);
    } else {
      // diff > 0，说明 enqueue_pos_ 已被其他线程前移，重新加载
      pos = enqueue_pos_.load(std::memory_order_relaxed);
    }
  } // end while()
}

/**
 * @brief 元素入队
 *
 * @tparam T 元素类型
 * @param value 元素
 */
template <typename T>
void LockfreeQueue<T>::push(T &&value) {
  size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
  while (true) {
    if (stopped_.load(std::memory_order_relaxed)) {
      return;
    }

    Slot &slot = buffer_[pos % capacity_];
    size_t seq = slot.sequence.load(std::memory_order_acquire);
    intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

    if (diff == 0) {
      if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
        assignToSlot(slot, std::move(value));
        slot.sequence.store(pos + 1, std::memory_order_release);
        return;
      }

    } else if (diff < 0) {
      std::this_thread::yield();
      pos = enqueue_pos_.load(std::memory_order_relaxed);
    } else {
      pos = enqueue_pos_.load(std::memory_order_relaxed);
    }
  } // end while()
}

/**
 * @brief 元素出队
 *
 * @tparam T 元素类型
 * @return std::optional<T> 元素
 */
template <typename T>
std::optional<T> LockfreeQueue<T>::pop() {
  size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
  while (true) {
    if (stopped_.load(std::memory_order_relaxed)) {
      return std::nullopt;
    }

    Slot &slot = buffer_[pos % capacity_];
    size_t seq = slot.sequence.load(std::memory_order_acquire);
    // 消费者需要等待生产者完成写入，所以是期待 pos + 1
    intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

    if (diff == 0) {
      // 槽位可读
      if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
        // 读取数据并移动出来
        T value = std::move(slot.data);
        // 更新序列号为 pos + capacity（用于绕回）
        slot.sequence.store(pos + capacity_, std::memory_order_release);
        return value;
      }
    } else if (diff < 0) {
      // 队列空，让出 CPU
      std::this_thread::yield();
      pos = dequeue_pos_.load(std::memory_order_relaxed);
    } else {
      // diff > 0，说明 dequeue_pos_ 已被前移
      pos = dequeue_pos_.load(std::memory_order_relaxed);
    }
  }
}

/**
 * @brief 判断队头元素
 *
 * @tparam T 元素类型
 * @return std::optional<T> 队头有元素可以出队，没有返回空
 */
template <typename T>
std::optional<T> LockfreeQueue<T>::tryPop() {
  size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
  Slot &slot = buffer_[pos % capacity_];
  size_t seq = slot.sequence.load(std::memory_order_acquire);
  intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

  if (diff == 0) {
    if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
      T value = std::move(slot.data);
      slot.sequence.store(pos + capacity_, std::memory_order_release);
      return value;
    }
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
bool LockfreeQueue<T>::empty() const {
  // 注意：由于并发，返回值可能瞬间过时，但作为近似可用
  size_t dq = dequeue_pos_.load(std::memory_order_acquire);
  size_t eq = enqueue_pos_.load(std::memory_order_acquire);
  return dq == eq;
}

/**
 * @brief 启动队列
 *
 */
template <typename T>
bool LockfreeQueue<T>::full() const {
  size_t eq = enqueue_pos_.load(std::memory_order_acquire);
  size_t dq = dequeue_pos_.load(std::memory_order_acquire);
  return (eq - dq) >= capacity_;
}

/**
 * @brief 停止队列
 */
template <typename T>
void LockfreeQueue<T>::start() {
  stopped_.store(false, std::memory_order_relaxed);
}

/**
 * @brief 判断队列满
 *
 * @return true 是的
 * @return false 不是
 */
template <typename T>
void LockfreeQueue<T>::stop() {
  stopped_.store(true, std::memory_order_relaxed);
}

/**
 * @brief 内部辅助函数：将值赋给槽位
 *
 * @param slot Slot 对象
 * @param value 值
 */
template <typename T>
template <typename U>
void hebpf::LockfreeQueue<T>::assignToSlot(Slot &slot, U &&value) {
  slot.data = std::forward<U>(value);
}

} // namespace hebpf
