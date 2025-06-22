#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include "spdlog/spdlog.h"

namespace hebpf {
namespace log {

constexpr std::string_view LOGNAME_DEFAULT{"hebpf"};

using LoggerType = spdlog::logger;
using LogLevel = spdlog::level::level_enum;

enum class Id { hebpf, loader, MAXSIZE };
enum class Level { trace, debug, info, warn, err, critical, off, MAXSIZE };

class Logger {
public:
  static std::shared_ptr<log::LoggerType> getLogger(const std::string &log_name);

private:
  static std::mutex mutex_;
  static std::unordered_map<std::string, std::shared_ptr<LoggerType>> map_;
};

template<Id id>
class Loggable {
public:
  virtual ~Loggable() = default;
  std::shared_ptr<log::LoggerType> getModuleLogger();
};

} // namespace log

#include "logger.tpp"

#define HEBPF_LEVEL(LEVEL) (static_cast<log::LogLevel>(log::Level::LEVEL))
#define HEBPF_LOGGER() log::Logger::getLogger(std::string{log::LOGNAME_DEFAULT})
#define HEBPF_LOG_TO(LOGGER, LEVEL, ...)                                                           \
  LOGGER->log(::spdlog::source_loc{__FILE__, __LINE__, __func__}, HEBPF_LEVEL(LEVEL), __VA_ARGS__)
#define HEBPF_LOG(LEVEL, ...) HEBPF_LOG_TO(HEBPF_LOGGER(), LEVEL, ##__VA_ARGS__)
#define HEBPF_MODULE_LOGGER() getModuleLogger()
#define HEBPF_MODULE_LOG(LEVEL, ...) HEBPF_LOG_TO(HEBPF_MODULE_LOGGER(), LEVEL, ##__VA_ARGS__)

} // namespace hebpf
