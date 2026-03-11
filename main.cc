// clang-format off
#include <csignal>
#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <thread>
#include "nlohmann/json.hpp"
#include "src/common/exception.h"
#include "src/daemon/configs.hpp"
#include "src/daemon/config_watcher.h"
#include "src/daemon/daemon.h"
#include "src/daemon/loader.h"
#include "src/data/queue.h"
#include "src/inotify/inotify.h"
#include "src/io/io.h"
#include "src/monitor/monitor.h"
#include "src/monitor/monitor_factory.h"
#include "src/monitor/monitor_subscriber.h"
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

    // 创建默认配置对象
    daemon::Configs config{};

    // 使用 YAML::Emitter 构造带注释的 YAML 字符串
    YAML::Emitter out{};
    out << YAML::BeginMap;

    out << YAML::Comment("以下是静态配置，仅在初始化时执行，不支持热更新");
    out << YAML::Key << daemon::CONFIGS_LOGPATH << YAML::Value << config.getLog();
    out << YAML::Key << daemon::CONFIGS_LOGLEVEL << YAML::Value
        << hebpf::enumName(config.getLogLevel());

    out << YAML::Newline;

    out << YAML::Comment("以下是支持热更新的配置");
    out << YAML::Key << daemon::CONFIGS_PROMETHEUS << YAML::Value << YAML::BeginMap;
    out << YAML::Key << daemon::CONFIGS_PROM_ENABLED << YAML::Value
        << config.getPrometheusEnabled();
    out << YAML::Key << daemon::CONFIGS_PROM_LISTEN << YAML::Value << config.getPrometheusListen();
    out << YAML::EndMap;

    auto ebpf_vec = config.getEbpf();
    if (!ebpf_vec.empty()) {
      out << YAML::Key << daemon::CONFIGS_EBPFSO << YAML::Value << ebpf_vec;
    }
    out << YAML::EndMap;

    auto configfile = std::string{daemon::CONFIGS_DEFAULT};
    std::ofstream fout{configfile};
    if (!fout) {
      GLOBAL_LOG(error, "Cannot create config file");
      return false;
    }
    fout << out.c_str(); // 输出 YAML 字符串
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

    daemon::Configs configs{daemon::CONFIGS_DEFAULT};
    log::LogConfig log_conf{};
    log_conf.setStdout(true);
    log_conf.setLogFile(configs.getLog());
    log_conf.setLevel(configs.getLogLevel());
    GLOBAL_LOG(info, "Launch " HEBPF_PROJECT " v" HEBPF_VERSION);

    auto io_ctx = std::make_shared<io::Io>();
    auto loader = std::make_unique<daemon::Loader>();
    auto daemon = std::make_shared<daemon::Daemon>(std::move(loader));
    if (!daemon->setIoContext(io_ctx)) {
      throw EXCEPT("Failed to setup io context");
    }

    auto watcher = std::make_shared<daemon::ConfigWatcher>(std::make_unique<inotify::Inotify>());
    if (!watcher->setIoContext(io_ctx)) {
      throw EXCEPT("Failed to setup io context");
    }
    watcher->attach(daemon);

    std::shared_ptr<monitor::MonitorSubscriber> monitor{};
    if (configs.getPrometheusEnabled()) {
      monitor = std::make_shared<monitor::MonitorSubscriber>(
          configs.getPrometheusListen(), std::make_shared<monitor::MonitorFactory>());

      auto queue = std::make_shared<Queue<nlohmann::json>>();
      if (queue == nullptr) {
        throw EXCEPT("Failed to setup status queue");
      }
      daemon->setStatusQueue(queue);
      monitor->setQueue(queue);
      watcher->attach(monitor);
      monitor->run();
    }

    watcher->startWatching(daemon::CONFIGS_DEFAULT);
    daemon->run();

    // 主循环
    while (!signal.shouldStop()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    daemon->stop();
    if (monitor != nullptr) {
      monitor->stop();
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
