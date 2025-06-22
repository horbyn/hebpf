#include "logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace hebpf {
namespace log {

std::mutex Logger::mutex_{};
std::unordered_map<std::string, std::shared_ptr<log::LoggerType>> Logger::map_{};

std::shared_ptr<log::LoggerType> Logger::getLogger(const std::string &log_name) {
  std::string log_name2 = log_name.empty() ? std::string{LOGNAME_DEFAULT} : log_name;

  std::lock_guard<std::mutex> lock(mutex_);
  if (map_.find(log_name2) == map_.end()) {
    auto temp = spdlog::stdout_color_mt(log_name2);
    temp->set_level(spdlog::level::trace);
    temp->flush_on(spdlog::level::trace);
    map_[log_name2] = temp;
  }
  return map_[log_name2];
}

} // namespace log
} // namespace hebpf
