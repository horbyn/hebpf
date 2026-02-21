/**
 * @file logger.h
 * @brief 日志模块是所有模块中第一个初始化的模块，因此除 common 外不应该依赖于项目其他模块
 *
 *        日志模块有两部分：
 *        - 全局日志：通过 GLOBAL_LOG() 宏触发，全局函数中使用
 *        - 模块日志：通过 LOG() 宏触发，任何继承了 log::Loggable
 *          的模块都可以使用
 * @date 2026-02-19
 *
 * @copyright Copyright (c) 2025
 */

#pragma once

// clang-format off
#include <memory>
#include <string>
#include <string_view>
#include "spdlog/spdlog.h"
#include "hebpf_version.h"
// clang-format on

namespace hebpf {
namespace log {

using LoggerType = spdlog::logger;
using LogLevel = spdlog::level::level_enum;

enum class Id : std::uint8_t { hebpf, ebpf, daemon, MAXSIZE };
enum class Level : std::uint8_t { trace, debug, info, warn, error, critical, off, MAXSIZE };

constexpr Level LOGLEVEL_DEFAULT{Level::trace};
constexpr std::string_view LOGFILE_DEFAULT{"/var/run/log/" HEBPF_PROJECT ".log"};

class LogConfig {
public:
  virtual ~LogConfig() = default;

  virtual void setLevel(Level level);
  virtual void setLevel(std::string_view level_str);
  virtual Level getLevel() const noexcept;
  virtual void setLogFile(std::string_view file);
  virtual void setStdout(bool set);

private:
  Level level_{LOGLEVEL_DEFAULT};
  std::string file_{LOGFILE_DEFAULT};
};

class Logger final {
public:
  static std::shared_ptr<log::LoggerType> getLogger(std::string_view log_name);
  static Level getLevel();
};

template <Id id>
class Loggable : public LogConfig {
public:
  virtual ~Loggable() = default;
  std::shared_ptr<log::LoggerType> getModuleLogger() const;
};

} // namespace log

#define LEVEL(LEV) (static_cast<log::LogLevel>(log::Level::LEV))
#define LOGGER() log::Logger::getLogger(HEBPF_PROJECT)
#define GLOBAL_LOG_TO(LOGGER, LEV, ...)                                                            \
  do {                                                                                             \
    if (static_cast<uint32_t>(log::Level::LEV) >=                                                  \
        static_cast<uint32_t>(log::Logger::getLevel())) {                                          \
      LOGGER->log(::spdlog::source_loc{__FILE__, __LINE__, __func__}, LEVEL(LEV), __VA_ARGS__);    \
    }                                                                                              \
  } while (0)
#define MODULE_LOGGER() getModuleLogger()
#define LOG_TO(LOGGER, LEV, ...)                                                                   \
  do {                                                                                             \
    if (static_cast<uint32_t>(log::Level::LEV) >= static_cast<uint32_t>(getLevel())) {             \
      LOGGER->log(::spdlog::source_loc{__FILE__, __LINE__, __func__}, LEVEL(LEV), __VA_ARGS__);    \
    }                                                                                              \
  } while (0)

// TODO: 有没有可能将两个日志宏统一起来？

#define GLOBAL_LOG(LEV, ...) GLOBAL_LOG_TO(LOGGER(), LEV, ##__VA_ARGS__)
#define LOG(LEV, ...) LOG_TO(MODULE_LOGGER(), LEV, ##__VA_ARGS__)

} // namespace hebpf

#include "logger.tpp"
