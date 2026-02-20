// clang-format off
#include <cstdlib>
#include <csignal>
#include <chrono>
#include <exception>
#include <iostream>
#include <thread>
#include "src/common/except.h"
#include "src/services/hello/hello.h"
// clang-format on

static volatile bool keep_running = true;
void signalHandler(int) { keep_running = false; }

int main() {
  using namespace hebpf::services::hello;

  try {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 打开 BPF skeleton
    HelloEbpf skeleton{};
    skeleton.open();
    if (!skeleton) {
      std::cerr << "Failed to open BPF skeleton" << "\n";
      return 1;
    }

    // 加载并验证 eBPF 程序
    skeleton.load();

    // 附加到挂载点
    skeleton.attach();

    std::cout << "eBPF program running! Press Ctrl+C to stop." << "\n";
    std::cout << "Run `sudo cat /sys/kernel/debug/tracing/trace_pipe` to view logs." << "\n";

    // 主循环
    while (keep_running) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    skeleton.detach();
    skeleton.destroy();
    std::cout << "\nShutting down" << "\n";
  } catch (const hebpf::except::Exception &exc) {
    std::cerr << "[error]: " << exc.what() << "\n"
              << "[stack frame]:" << "\n"
              << exc.stackFrame() << "\n";
    return EXIT_FAILURE;
  } catch (const std::exception &exc) {
    std::cerr << "[error] " << exc.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
