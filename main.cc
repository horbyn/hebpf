// clang-format off
#include <csignal>
#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <thread>
#include "nlohmann/json.hpp"
#include "src/cmdline/cmdline.h"
#include "src/common/enum_name.hpp"
#include "src/common/exception.h"
#include "src/daemon/configs.hpp"
#include "src/daemon/config_watcher.h"
#include "src/daemon/daemon.h"
#include "src/daemon/loader.h"
#include "src/data/queue.h"
#include "src/inotify/inotify_manager.h"
#include "src/io/io.h"
#include "src/monitor/monitor.h"
#include "src/monitor/monitor_factory.h"
#include "src/monitor/monitor_subscriber.h"
#include "src/signal/signal.h"
#include "hebpf_version.h"
// clang-format on

/**
 * @brief 获取空配置
 */
void generateEmptyConfigFile() {
  using namespace hebpf;

  daemon::Configs config{};

  YAML::Emitter out{};
  out << YAML::BeginMap;

  out << YAML::Key << daemon::CONFIGS_PROMETHEUS << YAML::Value << YAML::BeginMap;
  out << YAML::Key << daemon::CONFIGS_PROM_ENABLED << YAML::Value << config.getPrometheusEnabled();
  out << YAML::Key << daemon::CONFIGS_PROM_LISTEN << YAML::Value << config.getPrometheusListen();
  out << YAML::EndMap;

  out << YAML::Newline;
  out << YAML::Comment("以下是 eBPF 程序配置");
  out << YAML::Newline;

  out << YAML::Comment("ebpf:");
  out << YAML::Newline;

  out << YAML::Comment(" ebpf1:");
  out << YAML::Newline;
  out << YAML::Comment("   lib: /path/to/ebpf1.so");
  out << YAML::Newline;
  out << YAML::Comment("   config: /path/to/ebpf1.json");
  out << YAML::Newline;

  out << YAML::Comment(" ebpf2:");
  out << YAML::Newline;
  out << YAML::Comment("   lib: /path/to/ebpf2.so");
  out << YAML::Newline;
  out << YAML::Comment("   config: /path/to/ebpf2.json");
  out << YAML::Newline;

  auto ebpf_vec = config.getEbpfs();
  if (!ebpf_vec.empty()) {
    YAML::Node node{};
    node = ebpf_vec;
    out << YAML::Key << daemon::CONFIGS_EBPFSO << YAML::Value << node;
  }
  out << YAML::EndMap;

  auto configfile = std::string{daemon::CONFIGS_DEFAULT};
  std::ofstream fout{configfile};
  if (!fout) {
    std::cerr << "Cannot create config file\n";
    return;
  }
  fout << out.c_str();
  fout.close();

  std::cout << "Configuration " << daemon::CONFIGS_DEFAULT << " generated in current directory\n";
}

int main(int argc, char **argv) {
  using namespace hebpf;

  try {

    auto &signal = hebpf::signal::Signal::instance();
    signal.registerSignal(SIGINT, signal::Signal::commonSignalHandler);
    signal.registerSignal(SIGTERM, signal::Signal::commonSignalHandler);
    signal.registerSignal(SIGSEGV, signal::Signal::crashSignalHandler);

    cmdline::Cmdline cmd_parser{};
    auto [cmd_config, should_continue] = cmd_parser.parse(argc, argv);
    if (!should_continue) {
      std::cerr << "[error] Configuration parsing failed\n";
      return EXIT_FAILURE;
    }
    if (cmd_config.help_) {
      return EXIT_SUCCESS;
    }

    if (cmd_config.gen_empty_) {
      generateEmptyConfigFile();
      return EXIT_SUCCESS;
    }

    log::LogConfig log_conf{};
    log_conf.setStdout(true);
    log_conf.setLogFile(cmd_config.log_);
    log_conf.setLevel(cmd_config.loglevel_);
    GLOBAL_LOG(info, "Launch " HEBPF_PROJECT " v" HEBPF_VERSION);
    GLOBAL_LOG(info, "Log level: {}", enumName(log_conf.getLevel()));

    auto io_ctx = std::make_shared<io::Io>();
    auto inotify = std::make_shared<inotify::InotifyManager>();
    inotify->initManager(io_ctx);

    auto loader = std::make_unique<daemon::Loader>();
    if (!loader->setIoContext(io_ctx)) {
      throw EXCEPT("Failed to setup io context");
    }
    if (!loader->setInotifyManager(inotify)) {
      throw EXCEPT("Failed to setup inotify manager");
    }

    auto daemon = std::make_shared<daemon::Daemon>(std::move(loader));
    auto watcher = std::make_shared<daemon::ConfigWatcher>(inotify);
    watcher->attach(daemon);

    auto queue = std::make_shared<Queue<nlohmann::json>>();
    if (queue == nullptr) {
      throw EXCEPT("Failed to setup status queue");
    }

    daemon::Configs configs = cmd_config.config_;
    std::shared_ptr<monitor::MonitorSubscriber> monitor{};
    if (configs.getPrometheusEnabled()) {
      monitor = std::make_shared<monitor::MonitorSubscriber>(
          configs.getPrometheusListen(), std::make_unique<monitor::MonitorFactory>());
      monitor->setQueue(queue);
      watcher->attach(monitor);
      monitor->run();
    }

    watcher->startWatching(daemon::CONFIGS_DEFAULT);
    daemon->setStatusQueue(queue);
    daemon->run();

    // 主循环
    while (!signal.shouldStop()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    inotify->destroyManager();
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
