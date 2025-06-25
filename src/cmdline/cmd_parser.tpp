#pragma once

// clang-format off
#include <string>
#include "cmd_parser.h"
// clang-format on

namespace hebpf {
namespace cmdline {

/**
 * @brief 增加命令行选项
 *
 * @tparam T 选项类型
 * @param name 选项名称
 * @param variable 记录选项的值
 * @param description 选项描述
 * @param required 是否必须
 */
template <typename T>
auto CommandParser::add_option(const std::string &name, T &variable, const std::string &description,
                               bool required) -> void {
  auto opt = app_.add_option(name, variable, description);
  if (required) {
    opt->required();
  }
}

} // namespace cmdline
} // namespace hebpf
