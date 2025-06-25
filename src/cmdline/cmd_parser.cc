// clang-format off
#include "cmd_parser.h"
// clang-format on

namespace hebpf {
namespace cmdline {

CommandParser::CommandParser(const std::string &app_name) : app_(app_name) {}

/**
 * @brief 增加标识
 *
 * @param name 标识名称
 * @param variable 用来接收标识
 * @param description 标识描述
 */
auto CommandParser::add_flag(const std::string &name, bool &variable,
                             const std::string &description) -> void {
  app_.add_flag(name, variable, description);
}

/**
 * @brief 解析命令行参数
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数列表
 * @retval 0: 成功
 * @retval 非 0: 失败
 */
auto CommandParser::parse(int argc, char **argv) noexcept -> int {
  int ret = 0;
  try {
    app_.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    ret = app_.exit(e);
  }
  return ret;
}

} // namespace cmdline
} // namespace hebpf
