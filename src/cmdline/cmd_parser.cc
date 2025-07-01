// clang-format off
#include "cmd_parser.h"
#include <filesystem>
#include "CLI/CLI.hpp"
#include "src/except/except.h"
#include "hebpf_version.h"
// clang-format on

namespace hebpf {
namespace cmdline {

CommandParser::CommandParser(const std::string &app_name) : prog_name_(app_name) {}

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数列表
 * @retval 0: 成功
 * @retval 非 0: 失败
 */
auto CommandParser::parse(int argc, char **argv) -> int {
  namespace fs = std::filesystem;

  CLI::App app{prog_name_};
  app.set_version_flag("--version", std::string{log::LOGNAME_DEFAULT} + " v" + HEBPF_VERSION);

  fs::path log_path{};
  app.add_option("--logpath", log_path, "日志文件路径");
  std::string log_level{"trace"};
  app.add_option("--loglevel", log_level, "日志级别")
      ->check(CLI::IsMember({"trace", "debug", "info", "warn", "err", "critical", "off"}))
      ->default_val("info");

  int ret = 0;
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    ret = app.exit(e);
    if (ret == 0) {
      throw except::Success{};
    }
    return ret;
  }

  log::LogConfig log_config{};

  // TODO: 最后这些命令行参数与 config 模块交互
  if (fs::is_regular_file(log_path)) {
    ;
  } else if (fs::is_directory(log_path)) {
    log_path /= "hebpf.log";
  } else {
    log_path = log::LOGFILE_DEFAULT;
  }
  if (!log_path.empty()) {
    log_config.setLogFile(log_path.string());
  }

  log::Level level{};
  if (log_level == "trace") {
    level = log::Level::trace;
  } else if (log_level == "debug") {
    level = log::Level::debug;
  } else if (log_level == "info") {
    level = log::Level::info;
  } else if (log_level == "warn") {
    level = log::Level::warn;
  } else if (log_level == "err") {
    level = log::Level::err;
  } else if (log_level == "critical") {
    level = log::Level::critical;
  } else if (log_level == "off") {
    level = log::Level::off;
  } else {
    level = log::LOGLEVEL_DEFAULT;
  }
  log_config.setLevel(level);

  return ret;
}

} // namespace cmdline
} // namespace hebpf
