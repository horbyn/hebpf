#pragma once

#include <memory>
#include <string>

namespace log {

template<Id id>
std::shared_ptr<log::LoggerType> Loggable<id>::getModuleLogger() {
  std::string log_name{};
  // TODO: 将这个 switch 优化掉，可以 "自动" 根据 Id 枚举值生成 log_name
  switch (id) {
  case Id::hebpf: log_name = std::string{LOGNAME_DEFAULT}; break;
  case Id::loader: log_name = std::string{"loader"}; break;
  default: break;
  }
  return Logger::getLogger(log_name);
}

} // namespace log
