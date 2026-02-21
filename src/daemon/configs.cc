// clang-format off
#include "configs.hpp"
// clang-format on

namespace hebpf {
namespace daemon {

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

} // namespace daemon
} // namespace hebpf
