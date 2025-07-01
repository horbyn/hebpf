// clang-format off
#include <vector>
#include "benchmark/benchmark.h"
#include "src/cmdline/cmd_parser.h"
// clang-format on

static void BM_ParseSimpleCommand(benchmark::State &state) {
  hebpf::cmdline::CommandParser parser("bench_app");

  const char *argv[] = {"./bench_app", "--version"};
  int argc = sizeof(argv) / sizeof(argv[0]);

  for (auto _ : state) {
    try {
      auto ret = parser.parse(argc, const_cast<char **>(argv));
      benchmark::DoNotOptimize(ret);
    } catch (...) {
      // 吞掉异常保证测试继续
    }
  }
}
BENCHMARK(BM_ParseSimpleCommand);

static void BM_ParseFullCommand(benchmark::State &state) {
  hebpf::cmdline::CommandParser parser("bench_app");

  const char *argv[] = {"./bench_app", "--logpath", "/var/log/hebpf",
                        "--loglevel",  "debug",     "--version"};
  int argc = sizeof(argv) / sizeof(argv[0]);

  for (auto _ : state) {
    try {
      auto ret = parser.parse(argc, const_cast<char **>(argv));
      benchmark::DoNotOptimize(ret);
    } catch (...) {
      // 吞掉异常保证测试继续
    }
  }
}
BENCHMARK(BM_ParseFullCommand);

static void BM_ParseInvalidCommand(benchmark::State &state) {
  hebpf::cmdline::CommandParser parser("bench_app");

  const char *argv[] = {"./bench_app", "--invalid-flag"};
  int argc = sizeof(argv) / sizeof(argv[0]);

  for (auto _ : state) {
    try {
      auto ret = parser.parse(argc, const_cast<char **>(argv));
      benchmark::DoNotOptimize(ret);
    } catch (...) {
      // 吞掉异常保证测试继续
    }
  }
}
BENCHMARK(BM_ParseInvalidCommand);

BENCHMARK_MAIN();
