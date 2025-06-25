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

auto LogConfig::setLevel(Level level) -> void { g_level = level_ = level; }

auto LogConfig::getLevel() const noexcept -> Level { return level_; }

auto LogConfig::setLogFile(const std::string &file) -> void { g_logfile = file_ = file; }

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

} // namespace log
} // namespace hebpf
