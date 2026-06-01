// clang-format off
#include "configs.hpp"
#include <filesystem>
#include "src/fd/fd_if.h"
// clang-format on

namespace hebpf {
namespace daemon {

ConfigEbpf::ConfigEbpf()
    : lib_{"/path/to/libhebpf_xx.so"}, config_{"/path/to/config.json"}, hook_{HookType::UNKNOWN},
      ifindex_{FD_INVALID}, order_{0} {}

ConfigEbpf::ConfigEbpf(std::string_view lib, std::string_view config, HookType hook, int ifindex,
                       int order) {
  setLib(lib);
  setConfig(config);
  setHook(hook);
  setIfindex(ifindex);
  setOrder(order);
}

/**
 * @brief 设置 eBPF 配置动态库路径
 *
 * @param lib 动态库路径
 */
void ConfigEbpf::setLib(std::string_view lib) { lib_ = lib; }

/**
 * @brief 获取 eBPF 配置动态库路径
 *
 * @return std::string 动态库路径
 */
std::string ConfigEbpf::getLib() const { return lib_; }

/**
 * @brief 设置 eBPF 配置路径
 *
 * @param config 配置路径
 */
void ConfigEbpf::setConfig(std::string_view config) { config_ = config; }

/**
 * @brief 获取 eBPF 配置路径
 *
 * @return std::string 配置路径
 */
std::string ConfigEbpf::getConfig() const { return config_; }

/**
 * @brief 设置 hook 类型
 *
 * @param hook 类型
 */
void ConfigEbpf::setHook(HookType hook) { hook_ = hook; }

/**
 * @brief 获取 hook 类型
 *
 * @return HookType 类型
 */
HookType ConfigEbpf::getHook() const noexcept { return hook_; }

/**
 * @brief 设置网卡索引
 *
 * @param ifindex 网卡索引
 */
void ConfigEbpf::setIfindex(int ifindex) { ifindex_ = ifindex; }

/**
 * @brief 获取网卡索引
 *
 * @return int 网卡索引
 */
int ConfigEbpf::getIfindex() const noexcept { return ifindex_; }

/**
 * @brief 设置 eBPF 调用链顺序
 *
 * @param order 序号
 */
void ConfigEbpf::setOrder(int order) { order_ = order; }

/**
 * @brief 获取 eBPF 调用链顺序
 *
 * @return int 序号
 */
int ConfigEbpf::getOrder() const noexcept { return order_; }

bool ConfigEbpf::operator==(const ConfigEbpf &other) const {
  return lib_ == other.lib_ && config_ == other.config_ && hook_ == other.hook_ &&
         ifindex_ == other.ifindex_ && order_ == other.order_;
}

bool ConfigEbpf::operator!=(const ConfigEbpf &other) const { return !(*this == other); }

Configs::Configs(std::string_view configs_path) : Configs{} {
  try {
    *this = loadFromConfig(configs_path);
  } catch (const std::exception &exc) {
    LOG(warn, "Yaml configuration parse failed: {}", exc.what());
  }
}

/**
 * @brief 从配置文件解析配置
 * @note 用抛出异常来判断是否成功构造了 YAML 对象
 *
 * @param filepath 配置文件路径
 * @return Configs 配置对象
 */
Configs Configs::loadFromConfig(std::string_view filepath) {
  Configs configs{};

  namespace fs = std::filesystem;
  fs::path path{filepath};
  if (fs::is_regular_file(path) && !fs::is_empty(path)) {
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
 * @brief 设置 debug 服务器启用还是停用
 *
 * @param enabled 启用 true；停用 false
 */
void Configs::setDebugServerEnabled(bool enabled) { dbgserv_enabled_ = enabled; }

/**
 * @brief 获取 debug 服务器启用停用配置
 *
 * @return true 启用
 * @return false 停用
 */
bool Configs::getDebugServerEnabled() const { return dbgserv_enabled_; }

/**
 * @brief 设置 debug 服务器地址
 *
 * @param address 地址
 */
void Configs::setDebugServerAddr(std::string_view address) { dbgserv_addr_ = std::string{address}; }

/**
 * @brief 获取 debug 服务器地址
 *
 * @return std::string 地址
 */
std::string Configs::getDebugServerAddr() const { return dbgserv_addr_; }

/**
 * @brief 设置 debug 服务器端口
 *
 * @param port 端口
 */
void Configs::setDebugServerPort(uint16_t port) { dbgserv_port_ = port; }

/**
 * @brief 获取 debug 服务器端口
 *
 * @return uint16_t 端口
 */
uint16_t Configs::getDebugServerPort() const { return dbgserv_port_; }

/**
 * @brief 设置 eBPF 全量配置
 *
 * @param ebpf_so eBPF 全量配置
 */
void Configs::setEbpfs(const EbpfMap &ebpf_so) { ebpfs_ = ebpf_so; }

/**
 * @brief 获取 eBPF 全量配置
 *
 * @return Configs::EbpfMap 全量配置
 */
Configs::EbpfMap Configs::getEbpfs() const { return ebpfs_; }

/**
 * @brief 增加一个 eBPF 配置
 *
 * @param name eBPF 配置名称
 * @param lib eBPF 动态库路径
 * @param config eBPF 配置路径
 * @param hook hook 类型
 * @param ifindex 网卡索引
 * @param order eBPF 调用链顺序
 */
void Configs::appendEbpf(std::string_view name, std::string_view lib, std::string_view config,
                         HookType hook, int ifindex, int order) {
  if (name.empty()) {
    return;
  }

  std::string name_str{name};

  auto iter = ebpfs_.find(name_str);
  if (iter != ebpfs_.end()) {
    iter->second = ConfigEbpf{lib, config, hook, ifindex, order};
  } else {
    ebpfs_.emplace(name_str, ConfigEbpf{lib, config, hook, ifindex, order});
  }
}

/**
 * @brief 删除一个 eBPF 配置
 *
 * @param name eBPF 配置名称
 */
void Configs::deleteEbpf(std::string_view name) {
  if (name.empty()) {
    return;
  }

  auto iter = ebpfs_.find(std::string{name});
  if (iter != ebpfs_.end()) {
    ebpfs_.erase(iter);
  }
}

bool Configs::operator==(const Configs &other) const {
  return prometheus_enabled_ == other.prometheus_enabled_ &&
         prometheus_listen_ == other.prometheus_listen_ &&
         dbgserv_enabled_ == other.dbgserv_enabled_ && dbgserv_addr_ == other.dbgserv_addr_ &&
         dbgserv_port_ == other.dbgserv_port_ && ebpfs_ == other.ebpfs_;
}

bool Configs::operator!=(const Configs &other) const { return !(*this == other); }

} // namespace daemon
} // namespace hebpf
