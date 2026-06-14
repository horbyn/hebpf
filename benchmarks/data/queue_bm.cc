// clang-format off
#include <atomic>
#include <thread>
#include <vector>
#include "benchmark/benchmark.h"
#include "src/data/queue.h"
#include "src/data/lockfree_queue.h"
#include "src/data/lockfree_queue_boost.h"
// clang-format on

constexpr int QUEUE_CAPACITY = 1024;

/**
 * @brief 辅助函数：一个生产者线程的工作
 *
 * @param queue 队列
 * @param start_id 起始 ID
 * @param count 迭代数量
 * @param produced 已生产的数量
 */
template <typename Q>
void producer_work(Q &queue, int start_id, int count, std::atomic<size_t> &produced) {
  for (int i = 0; i < count; ++i) {
    queue.push(start_id + i);
  }
  produced.fetch_add(count, std::memory_order_release);
}

/**
 * @brief 辅助函数：消费者线程的工作
 *
 * @param queue 队列
 * @param expected_total 期待的总数
 * @param consumed 已消耗的数量
 */
template <typename Q>
void consumer_work(Q &queue, int expected_total, std::atomic<size_t> &consumed) {
  int received = 0;
  while (received < expected_total) {
    auto val = queue.pop();
    if (val != std::nullopt) {
      ++received;
    }
  }
  consumed.store(received, std::memory_order_release);
}

/**
 * @brief 通用基准测试实现
 *
 * @tparam Q 队列类型（有锁队列或无锁环形缓冲区队列）
 * @param state 框架上下文
 * @param producers 生产者线程数
 * @param total_ops 总操作数
 */
template <typename Q>
void RunBenchmark(benchmark::State &state, int producers, int total_ops) {
  // 确保 total_ops 可以被 producers 整除，否则调整
  int per_producer = total_ops / producers;
  if (per_producer * producers != total_ops) {
    state.SkipWithError("total_ops must be divisible by producers");
    return;
  }
  int total_expected = total_ops;

  for (auto _ : state) {
    Q queue{QUEUE_CAPACITY};
    queue.start();

    state.PauseTiming();

    std::atomic<size_t> produced{0};
    std::atomic<size_t> consumed{0};
    std::vector<std::thread> threads{};

    // 启动生产者线程
    for (int i = 0; i < producers; ++i) {
      threads.emplace_back(producer_work<Q>, std::ref(queue), i * per_producer, per_producer,
                           std::ref(produced));
    }
    // 启动消费者线程（固定 1 个）
    threads.emplace_back(consumer_work<Q>, std::ref(queue), total_expected, std::ref(consumed));

    state.ResumeTiming();

    // 等待所有线程完成
    for (auto &t : threads) {
      t.join();
    }

    queue.stop();

    // 检查是否所有元素都被消费了
    if (consumed.load() != static_cast<size_t>(total_expected)) {
      state.SkipWithError("Consumer didn't get all items");
      break;
    }
    state.SetItemsProcessed(total_ops);
  } // end for()
}

#define BENCH_CASE(QNAME, PRODUCERS, OPS)                                                          \
  static void BM_##QNAME##_##PRODUCERS##P_##OPS(benchmark::State &state) {                         \
    RunBenchmark<hebpf::QNAME<int>>(state, PRODUCERS, OPS);                                        \
  }                                                                                                \
  BENCHMARK(BM_##QNAME##_##PRODUCERS##P_##OPS)->Iterations(10)

// ========== 有锁队列测试用例 ==========
BENCH_CASE(Queue, 1, 10000);
BENCH_CASE(Queue, 1, 100000);
BENCH_CASE(Queue, 1, 1000000);
BENCH_CASE(Queue, 4, 10000);
BENCH_CASE(Queue, 4, 100000);
BENCH_CASE(Queue, 4, 1000000);
BENCH_CASE(Queue, 8, 10000);
BENCH_CASE(Queue, 8, 100000);
BENCH_CASE(Queue, 8, 1000000);

// ========== 自实现无锁队列测试用例 ==========
BENCH_CASE(LockfreeQueue, 1, 10000);
BENCH_CASE(LockfreeQueue, 1, 100000);
BENCH_CASE(LockfreeQueue, 1, 1000000);
BENCH_CASE(LockfreeQueue, 4, 10000);
BENCH_CASE(LockfreeQueue, 4, 100000);
BENCH_CASE(LockfreeQueue, 4, 1000000);
BENCH_CASE(LockfreeQueue, 8, 10000);
BENCH_CASE(LockfreeQueue, 8, 100000);
BENCH_CASE(LockfreeQueue, 8, 1000000);

// ========== Boost 无锁队列测试用例 ==========
BENCH_CASE(LockfreeQueueBoost, 1, 10000);
BENCH_CASE(LockfreeQueueBoost, 1, 100000);
BENCH_CASE(LockfreeQueueBoost, 1, 1000000);
BENCH_CASE(LockfreeQueueBoost, 4, 10000);
BENCH_CASE(LockfreeQueueBoost, 4, 100000);
BENCH_CASE(LockfreeQueueBoost, 4, 1000000);
BENCH_CASE(LockfreeQueueBoost, 8, 10000);
BENCH_CASE(LockfreeQueueBoost, 8, 100000);
BENCH_CASE(LockfreeQueueBoost, 8, 1000000);

BENCHMARK_MAIN();
