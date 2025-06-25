#pragma once

// clang-format off
#include <memory>
#include <string>
#include "logger.h"
// clang-format on

namespace hebpf {
namespace log {

/**
 * @brief 获取模块的日志对象
 *
 * @tparam id 模块 id
 * @return std::shared_ptr<log::LoggerType> 模块日志对象
 */
template <log::Id id> auto Loggable<id>::getModuleLogger() -> std::shared_ptr<log::LoggerType> {
  std::string log_name{};
  // TODO: 将这个 switch 优化掉，可以 "自动" 根据 Id 枚举值生成 log_name
  switch (id) {
  case Id::hebpf:
    log_name = std::string{LOGNAME_DEFAULT};
    break;
  case Id::cmdline:
    log_name = std::string{"cmdline"};
    break;
  default:
    break;
  }
  return Logger::getLogger(log_name);
}

} // namespace log
} // namespace hebpf
