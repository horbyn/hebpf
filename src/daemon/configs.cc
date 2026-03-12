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
 * @brief 设置 prometheus 启用还是停用
 *
 * @param enabled 启用 true；停用 false
 */
void Configs::setPrometheusEnabled(bool enabled) { prometheus_enabled_ = enabled; }

/**
 * @brief 获取 prometheus 启用停用配置
 *
 * @return true 启用
 * @return false 停用
 */
bool Configs::getPrometheusEnabled() const { return prometheus_enabled_; }

/**
 * @brief 设置 prometheus pull 地址
 *
 * @param listen 地址
 */
void Configs::setPrometheusListen(std::string_view listen) {
  prometheus_listen_ = std::string(listen);
}

/**
 * @brief 获取 prometheus pull 地址
 *
 * @return std::string 地址
 */
std::string Configs::getPrometheusListen() const { return prometheus_listen_; }

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
  return prometheus_enabled_ == other.prometheus_enabled_ &&
         prometheus_listen_ == other.prometheus_listen_ && ebpf_ == other.ebpf_;
}

bool Configs::operator!=(const Configs &other) const { return !(*this == other); }

} // namespace daemon
} // namespace hebpf
