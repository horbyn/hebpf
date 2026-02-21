// clang-format off
#include <cstdlib>
#include <csignal>
#include <chrono>
#include <exception>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <thread>
#include "src/common/exception.h"
#include "src/common/stack_frame.h"
#include "src/daemon/configs.hpp"
#include "src/daemon/loader.h"
#include "hebpf_version.h"
// clang-format on

static volatile bool keep_running = true;
void signalHandler(int) { keep_running = false; }
void crashHandler(int sig) {
  signalHandler(sig);

  constexpr std::string_view PROMPT{"Caught signal: "};
  write(STDERR_FILENO, PROMPT.data(), PROMPT.size());

  char sigbuf[32];
  int len = snprintf(sigbuf, sizeof(sigbuf), "%d\n", sig);
  write(STDERR_FILENO, sigbuf, len);

  hebpf::stackTrace(true);

  // 恢复默认信号
  signal(sig, SIG_DFL);
  raise(sig);
}

/**
 * @brief 获取空配置
 *
 * @param argc 命令行参数个数
 * @param argv 命令行选项
 * @return true 成功
 * @return false 失败
 */
bool generateEmptyConfigFile(int argc, char **argv) {
  using namespace hebpf;
  if (argc > 1 && std::string(argv[1]) == "--get-conf") {
    GLOBAL_LOG(info, "Generate empty config file");
    daemon::Configs config{};
    YAML::Node node{};
    node = config;
    auto configfile = std::string{daemon::CONFIGS_DEFAULT};
    std::ofstream fout{configfile};
    if (!fout) {
      GLOBAL_LOG(error, "Cannot create config file");
      return false;
    }
    fout << node;
    fout.close();

    GLOBAL_LOG(info, "Config file generated: {}", configfile);
    return true;
  }
  return false;
}

/**
 * @brief 从配置文件解析配置
 *
 * @return hebpf::daemon::Configs 配置对象
 */
hebpf::daemon::Configs parseCniConfig(std::string_view filepath) {
  using namespace hebpf;
  daemon::Configs configs{};

  namespace fs = std::filesystem;
  fs::path path{filepath};
  if (std::filesystem::is_regular_file(path)) {
    YAML::Node node = YAML::LoadFile(path.string());
    configs = node.as<daemon::Configs>();
  }

  return configs;
}

int main(int argc, char **argv) {
  using namespace hebpf;

  try {
    // 留个后门用来生成空的配置文件
    if (generateEmptyConfigFile(argc, argv)) {
      return EXIT_SUCCESS;
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGSEGV, crashHandler);

    daemon::Configs configs = parseCniConfig(daemon::CONFIGS_DEFAULT);

    log::LogConfig log_conf{};
    log_conf.setStdout(true);
    log_conf.setLogFile(configs.getLog());
    log_conf.setLevel(configs.getLogLevel());
    GLOBAL_LOG(info, "Launch " HEBPF_PROJECT " v" HEBPF_VERSION);

    daemon::Loader loader{};

    const auto &ebpf = configs.getEbpf();
    for (const auto &so_path : ebpf) {
      std::string so_name = loader.loadService(so_path);
      auto *service = loader.getService(so_name);
      if (service == nullptr) {
        throw EXCEPT(fmt::format("Cannot load service: ", so_name));
      }

      // 打开 BPF skeleton
      service->open();
      if (!service) {
        std::cerr << "Failed to open BPF service" << "\n";
        return 1;
      }

      // 加载并验证 eBPF 程序
      service->load();

      // 附加到挂载点
      service->attach();
    }

    GLOBAL_LOG(info, "eBPF program running! Press Ctrl+C to stop");
    GLOBAL_LOG(info, "Run `sudo cat /sys/kernel/debug/tracing/trace_pipe` to view logs");

    // 主循环
    while (keep_running) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    const auto &all_ebpf = loader.getAllService();
    for (const auto &so_name : all_ebpf) {
      auto *service = loader.getService(so_name);
      service->detach();
      service->destroy();
    }

    GLOBAL_LOG(info, "Shutting down");
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
