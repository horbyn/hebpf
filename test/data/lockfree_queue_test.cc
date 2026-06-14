// clang-format off
#include <algorithm>
#include <atomic>
#include <set>
#include <thread>
#include <vector>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "src/data/lockfree_queue.h"
// clang-format on

using namespace hebpf;

/**
 * @brief 测试空队列和基本操作
 *
 */
TEST(LockfreeQueueTest, InitiallyEmpty) {
  LockfreeQueue<int> queue{16};
  queue.start();
  EXPECT_TRUE(queue.empty());
  EXPECT_FALSE(queue.full());
  EXPECT_EQ(queue.tryPop(), std::nullopt);
  queue.stop();
}

/**
 * @brief 测试单次 push、pop
 *
 */
TEST(LockfreeQueueTest, PushPopSingle) {
  LockfreeQueue<int> queue{16};
  queue.start();
  queue.push(42);
  EXPECT_FALSE(queue.empty());
  auto val = queue.pop();
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, 42);
  EXPECT_TRUE(queue.empty());
  queue.stop();
}

/**
 * @brief 测试移动 push
 *
 */
TEST(LockfreeQueueTest, PushPopMove) {
  LockfreeQueue<std::string> queue{16};
  queue.start();
  std::string s = "hello";
  queue.push(std::move(s));
  EXPECT_TRUE(s.empty()); // 已被移动
  auto val = queue.pop();
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "hello");
  queue.stop();
}

/**
 * @brief 测试 try pop
 *
 */
TEST(LockfreeQueueTest, TryPopNonBlocking) {
  LockfreeQueue<int> queue{16};
  queue.start();
  EXPECT_EQ(queue.tryPop(), std::nullopt);
  queue.push(100);
  auto val = queue.tryPop();
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, 100);
  EXPECT_TRUE(queue.empty());
  queue.stop();
}

/**
 * @brief 验证 SPSC 顺序和数量
 *
 */
TEST(LockfreeQueueTest, SingleProducerSingleConsumer) {
  const int num_items = 10000;
  LockfreeQueue<int> queue{1024};
  queue.start();

  std::atomic<size_t> produced{0};
  std::atomic<size_t> consumed{0};

  std::thread producer([&] {
    for (int i = 0; i < num_items; ++i) {
      queue.push(i);
      produced.fetch_add(1, std::memory_order_release); // 每生产一个就计数
    }
  });

  std::thread consumer([&] {
    int expected = 0;
    while (expected < num_items) {
      auto val = queue.pop();
      if (val) {
        EXPECT_EQ(*val, expected);
        ++expected;
        consumed.fetch_add(1, std::memory_order_release); // 没消耗一个就计数
      }
    }
  });

  producer.join();
  consumer.join();
  queue.stop();

  EXPECT_EQ(produced.load(), num_items);
  EXPECT_EQ(consumed.load(), num_items);
}

/**
 * @brief MPSC 测试
 *
 */
TEST(LockfreeQueueTest, MultiProducerSingleConsumer) {
  const int num_producers = 4;
  const int items_per_producer = 5000;
  const int total_items = num_producers * items_per_producer;
  LockfreeQueue<int> queue{total_items + 1};
  queue.start();

  std::atomic<size_t> produced{0};
  std::atomic<size_t> consumed{0};
  std::vector<std::thread> producers{};
  std::vector<std::vector<int>> produced_values(num_producers);

  for (int p = 0; p < num_producers; ++p) {
    producers.emplace_back([&, p] {
      std::vector<int> local{};
      local.reserve(items_per_producer);
      for (int i = 0; i < items_per_producer; ++i) {
        int val = p * items_per_producer + i;
        queue.push(val);
        local.push_back(val);
      }
      produced_values[p] = std::move(local);
      produced.fetch_add(items_per_producer, std::memory_order_release);
    });
  }

  std::thread consumer([&] {
    // 使用 std::multiset 来接收的原因是：
    // 只要元素及其出现次数完全相同就相等，不需要关心接收顺序
    std::multiset<int> received{};
    while (static_cast<int>(received.size()) < total_items) {
      auto val = queue.pop();
      if (val) {
        received.insert(*val);
      }
    }
    consumed.store(total_items, std::memory_order_release);
    // 验证接收到的集合与期望的集合一致
    std::multiset<int> expected{};
    for (const auto &vec : produced_values) {
      expected.insert(vec.begin(), vec.end());
    }
    EXPECT_EQ(received, expected);
  });

  for (auto &t : producers) {
    t.join();
  }
  consumer.join();
  queue.stop();

  EXPECT_EQ(produced.load(), total_items);
  EXPECT_EQ(consumed.load(), total_items);
}

/**
 * @brief 压力测试：随机并发 push/pop
 *
 */
TEST(LockfreeQueueTest, StressTest) {
  const int num_producers = 4;
  const int items_per_producer = 10000;
  const int total_items = num_producers * items_per_producer;
  LockfreeQueue<int> queue{total_items + 1};
  queue.start();

  std::atomic<size_t> produced{0};
  std::atomic<size_t> consumed{0};
  std::vector<std::thread> producers{};
  std::vector<std::vector<int>> produced_values(num_producers);

  for (int p = 0; p < num_producers; ++p) {
    producers.emplace_back([&, p] {
      std::vector<int> local{};
      local.reserve(items_per_producer);
      for (int i = 0; i < items_per_producer; ++i) {
        int val = p * items_per_producer + i;
        queue.push(val);
        local.push_back(val);
        // 偶尔让出 CPU 模拟真实场景
        if (i % 100 == 0)
          std::this_thread::yield();
      }
      produced_values[p] = std::move(local);
      produced.fetch_add(items_per_producer, std::memory_order_release);
    });
  }

  std::thread consumer([&] {
    std::multiset<int> received{};
    while (static_cast<int>(received.size()) < total_items) {
      auto val = queue.tryPop(); // 使用 tryPop 非阻塞，增加随机性
      if (val) {
        received.insert(*val);
      } else {
        std::this_thread::yield();
      }
    }
    consumed.store(total_items, std::memory_order_release);
    // 验证完整性
    std::multiset<int> expected{};
    for (const auto &vec : produced_values) {
      expected.insert(vec.begin(), vec.end());
    }
    EXPECT_EQ(received, expected);
  });

  for (auto &t : producers) {
    t.join();
  }
  consumer.join();
  queue.stop();

  EXPECT_EQ(produced.load(), total_items);
  EXPECT_EQ(consumed.load(), total_items);
}

/**
 * @brief 测试停止标志
 *
 */
TEST(LockfreeQueueTest, StopBehavior) {
  LockfreeQueue<int> queue{16};
  queue.start();
  queue.stop();
  // 停止后 push/pop 应直接返回
  queue.push(1);          // 应直接返回，不会阻塞
  auto val = queue.pop(); // 应立即返回 nullopt
  EXPECT_FALSE(val.has_value());
  EXPECT_TRUE(queue.empty());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
