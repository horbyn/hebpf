// clang-format off
#include "logger.h"
#include <atomic>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "src/common/enum_name.hpp"
// clang-format on

namespace hebpf {
namespace log {

static std::mutex g_mutex{};
static std::unordered_map<std::string, std::shared_ptr<log::LoggerType>> g_map{};
static Level g_level{LOGLEVEL_DEFAULT};
static std::string g_logfile{LOGFILE_DEFAULT};
static std::atomic<bool> g_stdout{false};

/**
 * @brief 设置日志等级
 *
 * @param level 日志等级
 */
void LogConfig::setLevel(Level level) {
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_level = level;
  }
}

/**
 * @brief 设置日志等级
 *
 * @param level_str 日志等级字符串
 */
void LogConfig::setLevel(std::string_view level_str) {
  GLOBAL_LOG(info, "日志等级: {}", level_str);
  auto level_opt = stringEnum<log::Level>(level_str);
  log::Level level = level_opt.has_value() ? level_opt.value() : log::LOGLEVEL_DEFAULT;
  setLevel(level);
}

/**
 * @brief 获取日志等级
 *
 * @return Level 日志等级
 */
Level LogConfig::getLevel() const noexcept { return Logger::getLevel(); }

/**
 * @brief 设置日志文件路径
 *
 * @param file 日志文件路径
 */
void LogConfig::setLogFile(std::string_view file) {
  if (!file.empty()) {
    GLOBAL_LOG(info, "日志文件: {}", file);
  }

  namespace fs = std::filesystem;
  fs::path log_path =
      file.empty() ? fs::path{std::string{log::LOGFILE_DEFAULT}} : fs::path{std::string{file}};
  if (fs::is_directory(log_path)) {
    log_path /= HEBPF_PROJECT ".log";
  }

  file_ = log_path.string();
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_logfile = file_;
  }
}

/**
 * @brief 设置标准输出日志
 *
 * @param set 日志输出 stdout（true）日志不输出 stdout（false）
 */
void LogConfig::setStdout(bool set) { g_stdout.store(set); }

/**
 * @brief 获取全局的日志对象
 *
 * @param log_name 日志模块名称
 * @return std::shared_ptr<log::LoggerType> 全局日志对象
 */
std::shared_ptr<log::LoggerType> Logger::getLogger(std::string_view log_name) {
  std::string log_name2 = log_name.empty() ? std::string{HEBPF_PROJECT} : std::string{log_name};

  std::lock_guard<std::mutex> lock(g_mutex);
  if (g_map.find(log_name2) == g_map.end()) {
    constexpr auto MAX_SIZE = 4 * 1024 * 1024;
    constexpr auto MAX_FILES = 5;
    auto file_sink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(g_logfile, MAX_SIZE, MAX_FILES);
    std::vector<spdlog::sink_ptr> sinks{file_sink};

    if (g_stdout.load()) {
      auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      sinks.emplace_back(stdout_sink);
    }

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
Level Logger::getLevel() {
  Level ret{};
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    ret = g_level;
  }
  return ret;
}

} // namespace log
} // namespace hebpf
