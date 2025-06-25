// clang-format off
#include <cstdlib>
#include "src/log/logger.h"
#include "src/cmdline/cmd_parser.h"
// clang-format on

auto main(int argc, char **argv) -> int {
  using namespace hebpf;

  cmdline::CommandParser parser{"hebpf"};

  std::string log_path{};
  parser.add_option("-l,--logpath", log_path, "日志文件路径");
  log::Level log_level{log::LOGLEVEL_DEFAULT};
  parser.add_option("-g,--loglevel", log_level, "日志级别");

  auto ret = parser.parse(argc, argv);
  if (ret != 0) {
    return EXIT_FAILURE;
  }

  log::LogConfig log_config{};
  if (!log_path.empty()) {
    log_config.setLogFile(log_path);
  }
  log_config.setLevel(log_level);

  HEBPF_GLOBAL_LOG(info, "启动 hebpf");
  return EXIT_SUCCESS;
}
