#pragma once

// clang-format off
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include "spdlog/spdlog.h"
// clang-format on

namespace hebpf {
namespace log {

using LoggerType = spdlog::logger;
using LogLevel = spdlog::level::level_enum;

enum class Id : std::uint8_t { hebpf, cmdline, MAXSIZE };
enum class Level : std::uint8_t { trace, debug, info, warn, err, critical, off, MAXSIZE };

constexpr std::string_view LOGNAME_DEFAULT{"hebpf"};
constexpr Level LOGLEVEL_DEFAULT{Level::trace};
constexpr std::string_view LOGFILE_DEFAULT{"/dev/null"};

class LogConfig {
public:
  virtual ~LogConfig() = default;

  virtual auto setLevel(Level level) -> void;
  virtual auto getLevel() const noexcept -> Level;
  virtual auto setLogFile(const std::string &file) -> void;

private:
  Level level_{LOGLEVEL_DEFAULT};
  std::string file_{LOGFILE_DEFAULT};
};

class Logger final {
public:
  static auto getLogger(const std::string &log_name) -> std::shared_ptr<log::LoggerType>;
  static auto getLevel() -> Level;
};

template <Id id> class Loggable : public LogConfig {
public:
  virtual ~Loggable() = default;
  auto getModuleLogger() -> std::shared_ptr<log::LoggerType>;
};

} // namespace log

#define HEBPF_LEVEL(LEVEL) (static_cast<log::LogLevel>(log::Level::LEVEL))
#define HEBPF_LOGGER() log::Logger::getLogger(std::string{log::LOGNAME_DEFAULT})
#define HEBPF_GLOBAL_LOG_TO(LOGGER, LEVEL, ...)                                                    \
  do {                                                                                             \
    if (static_cast<uint32_t>(log::Level::LEVEL) >=                                                \
        static_cast<uint32_t>(log::Logger::getLevel())) {                                          \
      LOGGER->log(::spdlog::source_loc{__FILE__, __LINE__, __func__}, HEBPF_LEVEL(LEVEL),          \
                  __VA_ARGS__);                                                                    \
    }                                                                                              \
  } while (0)
#define HEBPF_MODULE_LOGGER() getModuleLogger()
#define HEBPF_LOG_TO(LOGGER, LEVEL, ...)                                                           \
  do {                                                                                             \
    if (static_cast<uint32_t>(log::Level::LEVEL) >= static_cast<uint32_t>(getLevel())) {           \
      LOGGER->log(::spdlog::source_loc{__FILE__, __LINE__, __func__}, HEBPF_LEVEL(LEVEL),          \
                  __VA_ARGS__);                                                                    \
    }                                                                                              \
  } while (0)

// TODO: 有没有可能将两个日志宏统一起来？

#define HEBPF_GLOBAL_LOG(LEVEL, ...) HEBPF_GLOBAL_LOG_TO(HEBPF_LOGGER(), LEVEL, ##__VA_ARGS__)
#define HEBPF_LOG(LEVEL, ...) HEBPF_LOG_TO(HEBPF_MODULE_LOGGER(), LEVEL, ##__VA_ARGS__)

} // namespace hebpf

#include "logger.tpp"
