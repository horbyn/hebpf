// clang-format off
#include "logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
// clang-format on

namespace hebpf {
namespace log {

static std::mutex g_mutex{};
static std::unordered_map<std::string, std::shared_ptr<log::LoggerType>> g_map{};
static Level g_level{LOGLEVEL_DEFAULT};
static std::string g_logfile{LOGFILE_DEFAULT};

/**
 * @brief 设置日志等级
 *
 * @param level 日志等级
 */
auto LogConfig::setLevel(Level level) -> void {
  level_ = level;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_level = level;
  }
}

/**
 * @brief 获取日志等级
 *
 * @return Level 日志等级
 */
auto LogConfig::getLevel() const noexcept -> Level { return level_; }

/**
 * @brief 设置日志文件路径
 *
 * @param file 日志文件路径
 */
auto LogConfig::setLogFile(const std::string &file) -> void {
  file_ = file;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_logfile = file;
  }
}

/**
 * @brief 获取全局的日志对象
 *
 * @param log_name 日志模块名称
 * @return std::shared_ptr<log::LoggerType> 全局日志对象
 */
auto Logger::getLogger(const std::string &log_name) -> std::shared_ptr<log::LoggerType> {
  std::string log_name2 = log_name.empty() ? std::string{LOGNAME_DEFAULT} : log_name;

  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_map.find(log_name2) == g_map.end()) {
    constexpr auto MAX_SIZE = 4 * 1024 * 1024;
    constexpr auto MAX_FILES = 5;
    auto file_sink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(g_logfile, MAX_SIZE, MAX_FILES);
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    std::vector<spdlog::sink_ptr> sinks{file_sink, stdout_sink};
    auto temp = std::make_shared<spdlog::logger>(log_name2, sinks.begin(), sinks.end());
    temp->set_level(static_cast<spdlog::level::level_enum>(g_level));
    temp->flush_on(static_cast<spdlog::level::level_enum>(g_level));

    g_map[log_name2] = temp;
  }
  return g_map[log_name2];
}

/**
 * @brief 获取日志等级
 *
 * @return Level 日志等级
 */
auto Logger::getLevel() -> Level {
  Level ret{};
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    ret = g_level;
  }
  return ret;
}

} // namespace log
} // namespace hebpf
