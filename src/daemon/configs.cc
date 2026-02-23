// clang-format off
#include "configs.hpp"
#include <filesystem>
// clang-format on

namespace hebpf {
namespace daemon {

Configs::Configs(std::string_view configs_path) : Configs{} {
  try {
    *this = loadFromConfig(configs_path);
  } catch (const std::exception &exc) {
    LOG(warn, "Yaml configuration parse failed: {}", exc.what());
  }
}

/**
 * @brief 从配置文件解析配置
 *
 * @return Configs 配置对象
 */
Configs Configs::loadFromConfig(std::string_view filepath) {
  Configs configs{};

  namespace fs = std::filesystem;
  fs::path path{filepath};
  if (fs::is_regular_file(path)) {
    YAML::Node node = YAML::LoadFile(path.string());
    configs = node.as<daemon::Configs>();
  }

  return configs;
}

/**
 * @brief 设置日志文件路径
 *
 * @param logpath 日志文件路径
 */
void Configs::setLog(std::string_view logpath) { log_ = std::string{logpath}; }

/**
 * @brief 获取日志文件路径
 *
 * @return std::string 日志文件路径
 */
std::string Configs::getLog() const { return log_; }

/**
 * @brief 设置日志级别
 *
 * @param level 日志级别
 */
void Configs::setLogLevel(log::Level level) { loglevel_ = level; }

/**
 * @brief 获取日志级别
 *
 * @return log::Level 日志级别
 */
log::Level Configs::getLogLevel() const { return loglevel_; }

/**
 * @brief 设置 eBPF 程序路径
 *
 * @param ebpf_so 路径
 */
void Configs::setEbpf(const std::vector<std::string> &ebpf_so) { ebpf_ = ebpf_so; }

/**
 * @brief 获取 eBPF 程序路径
 *
 * @return std::vector<std::string> 路径
 */
std::vector<std::string> Configs::getEbpf() const { return ebpf_; }

bool Configs::operator==(const Configs &other) const {
  return log_ == other.log_ && loglevel_ == other.loglevel_ && ebpf_ == other.ebpf_;
}

bool Configs::operator!=(const Configs &other) const { return !(*this == other); }

} // namespace daemon
} // namespace hebpf
