// clang-format off
#include <algorithm>
#include <atomic>
#include <set>
#include <thread>
#include <vector>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "src/data/lockfree_queue_boost.h"
// clang-format on

using namespace hebpf;

TEST(LockfreeQueueBoostTest, InitiallyEmpty) {
  LockfreeQueueBoost<int> queue{16};
  queue.start();
  EXPECT_TRUE(queue.empty());
  EXPECT_FALSE(queue.full());
  EXPECT_EQ(queue.tryPop(), std::nullopt);
  queue.stop();
}

TEST(LockfreeQueueBoostTest, PushPopSingle) {
  LockfreeQueueBoost<int> queue{16};
  queue.start();
  queue.push(42);
  EXPECT_FALSE(queue.empty());
  auto val = queue.pop();
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, 42);
  EXPECT_TRUE(queue.empty());
  queue.stop();
}

TEST(LockfreeQueueBoostTest, TryPopNonBlocking) {
  LockfreeQueueBoost<int> queue{16};
  queue.start();
  EXPECT_EQ(queue.tryPop(), std::nullopt);
  queue.push(100);
  auto val = queue.tryPop();
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, 100);
  EXPECT_TRUE(queue.empty());
  queue.stop();
}

TEST(LockfreeQueueBoostTest, FullQueue) {
  const size_t capacity = 8;
  LockfreeQueueBoost<int> queue{capacity};
  queue.start();
  for (size_t i = 0; i < capacity; ++i) {
    queue.push(i);
  }
  EXPECT_FALSE(queue.full());
  queue.push(42);
  EXPECT_FALSE(queue.full());
  auto val = queue.tryPop();
  ASSERT_TRUE(val.has_value());
  EXPECT_NE(*val, 42); // 队列满之后插入失败
  queue.stop();
}

TEST(LockfreeQueueBoostTest, SingleProducerSingleConsumer) {
  const int num_items = 10000;
  LockfreeQueueBoost<int> queue{1024};
  queue.start();

  std::atomic<size_t> produced{0};
  std::atomic<size_t> consumed{0};

  std::thread producer([&] {
    for (int i = 0; i < num_items; ++i) {
      queue.push(i);
      produced.fetch_add(1, std::memory_order_release);
    }
  });

  std::thread consumer([&] {
    int expected = 0;
    while (expected < num_items) {
      auto val = queue.pop();
      if (val) {
        EXPECT_EQ(*val, expected);
        ++expected;
        consumed.fetch_add(1, std::memory_order_release);
      }
    }
  });

  producer.join();
  consumer.join();
  queue.stop();

  EXPECT_EQ(produced.load(), num_items);
  EXPECT_EQ(consumed.load(), num_items);
}

TEST(LockfreeQueueBoostTest, MultiProducerSingleConsumer) {
  const int num_producers = 4;
  const int items_per_producer = 5000;
  const int total_items = num_producers * items_per_producer;
  LockfreeQueueBoost<int> queue{total_items / 4}; // 使用更小容量的队列
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
        queue.push(val); // 生产者可能很快堆满队列
        local.push_back(val);
      }
      produced_values[p] = std::move(local);
      produced.fetch_add(items_per_producer, std::memory_order_release);
    });
  }

  std::thread consumer([&] {
    std::multiset<int> received{};
    while (static_cast<int>(received.size()) < total_items) {
      auto val = queue.pop(); // 消费者不一定能及时消费
      if (val) {
        received.insert(*val);
      }
    }
    consumed.store(total_items, std::memory_order_release);
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

TEST(LockfreeQueueBoostTest, StressTest) {
  const int num_producers = 4;
  const int items_per_producer = 10000;
  const int total_items = num_producers * items_per_producer;
  LockfreeQueueBoost<int> queue(total_items / 4); // 也是使用更小容量的队列
  queue.start();

  std::atomic<size_t> produced{0};
  std::atomic<size_t> consumed{0};
  std::vector<std::thread> producers{};
  std::vector<std::vector<int>> produced_values(num_producers);

  for (int p = 0; p < num_producers; ++p) {
    producers.emplace_back([&, p] {
      std::vector<int> local;
      local.reserve(items_per_producer);
      for (int i = 0; i < items_per_producer; ++i) {
        int val = p * items_per_producer + i;
        queue.push(val);
        local.push_back(val);
        if (i % 100 == 0) {
          std::this_thread::yield(); // 模拟真实环境，生产者写入队列后做一些业务处理
        }
      }
      produced_values[p] = std::move(local);
      produced.fetch_add(items_per_producer, std::memory_order_release);
    });
  }

  std::thread consumer([&] {
    std::multiset<int> received{};
    while (static_cast<int>(received.size()) < total_items) {
      auto val = queue.tryPop();
      if (val) {
        received.insert(*val);
      } else {
        std::this_thread::yield();
      }
    }
    consumed.store(total_items, std::memory_order_release);
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

TEST(LockfreeQueueBoostTest, StopBehavior) {
  LockfreeQueueBoost<int> queue{16};
  queue.start();
  queue.stop();
  queue.push(1);
  auto val = queue.pop();
  EXPECT_FALSE(val.has_value());
  EXPECT_TRUE(queue.empty());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
