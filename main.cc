// clang-format off
#include <csignal>
#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <thread>
#include "src/common/exception.h"
#include "src/daemon/configs.hpp"
#include "src/daemon/config_watcher.h"
#include "src/daemon/daemon.h"
#include "src/daemon/loader.h"
#include "src/signal/signal.h"
#include "hebpf_version.h"
// clang-format on

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

int main(int argc, char **argv) {
  using namespace hebpf;

  try {
    // 留个后门用来生成空的配置文件
    if (generateEmptyConfigFile(argc, argv)) {
      GLOBAL_LOG(info, "Configuration {} generated in current directory", daemon::CONFIGS_DEFAULT);
      return EXIT_SUCCESS;
    }

    auto &signal = hebpf::signal::Signal::instance();
    signal.registerSignal(SIGINT, signal::Signal::commonSignalHandler);
    signal.registerSignal(SIGTERM, signal::Signal::commonSignalHandler);
    signal.registerSignal(SIGSEGV, signal::Signal::crashSignalHandler);

    // 这是根据配置文件先进行初始化，后续是可以热更新的
    // TODO: 热更新目前只支持 eBPF 程序
    daemon::Configs configs{daemon::CONFIGS_DEFAULT};
    log::LogConfig log_conf{};
    log_conf.setStdout(true);
    log_conf.setLogFile(configs.getLog());
    log_conf.setLevel(configs.getLogLevel());
    GLOBAL_LOG(info, "Launch " HEBPF_PROJECT " v" HEBPF_VERSION);

    auto loader = std::make_unique<daemon::Loader>();
    daemon::Daemon daemon{std::move(loader)};
    daemon.run();

    daemon::ConfigWatcher watcher{daemon::CONFIGS_DEFAULT, signal};
    watcher.start(std::bind(&daemon::Daemon::OnConfigChanged, &daemon, std::placeholders::_1));

    // 主循环
    while (!signal.shouldStop()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    watcher.stop();
    daemon.stop();

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
