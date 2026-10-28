// clang-format off
#include "configs.hpp"
#include <filesystem>
#include "spdlog/fmt/ranges.h"
#include "src/common/assert.h"
#include "src/common/enum_name.hpp"
#include "src/fd/fd_if.h"
// clang-format on

namespace hebpf {
namespace daemon {

ConfigEbpf::ConfigEbpf()
    : lib_{}, config_{}, ifindex_{FD_INVALID}, order_{0}, prog_id_{0}, name_{} {}

ConfigEbpf::ConfigEbpf(std::string_view lib, std::string_view config, int ifindex, int order,
                       ProgId prog_id, std::string_view name) {
  setLib(lib);
  setConfig(config);
  setIfindex(ifindex);
  setOrder(order);
  setProgId(prog_id);
  setName(name);
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

/**
 * @brief 设置程序唯一标识
 *
 * @param prog_id 唯一标识
 */
void ConfigEbpf::setProgId(ProgId prog_id) { prog_id_ = prog_id; }

/**
 * @brief 获取程序唯一标识
 *
 * @return ProgId 唯一标识
 */
ProgId ConfigEbpf::getProgId() const noexcept { return prog_id_; }

/**
 * @brief 设置程序名称
 *
 * @param name 名称
 */
void ConfigEbpf::setName(std::string_view name) { name_ = std::string{name}; }

/**
 * @brief 获取程序名称
 *
 * @return std::string 名称
 */
std::string ConfigEbpf::getName() const { return name_; }

bool ConfigEbpf::operator==(const ConfigEbpf &other) const {
  return lib_ == other.lib_ && config_ == other.config_ && ifindex_ == other.ifindex_ &&
         order_ == other.order_ && prog_id_ == other.prog_id_ && name_ == other.name_;
}

bool ConfigEbpf::operator!=(const ConfigEbpf &other) const { return !(*this == other); }

/**
 * @brief 返回调度器名称
 * @param hook_type 绑定类型
 *
 * @note 格式为 \"SCHEDULER_XDP_Gen\" 或 \"SCHEDULER_TC\" 等
 *
 * @return std::string 调度器名称
 */
std::string ConfigEbpf::getSchedulerName(HookType hook_type) {
  return fmt::format("{}_{}", ID_SCHEDULER_PREFIX, enumName(hook_type));
}

/**
 * @brief 判断是否为调度器
 *
 * @param name eBPF 程序名称
 * @return true 是
 * @return false 不是
 */
bool ConfigEbpf::isScheduler(std::string_view name) {
  return name.find(ID_SCHEDULER_PREFIX) != std::string::npos;
}

/**
 * @brief 是否是网络 hook
 *
 * @param hook_type 绑定类型
 * @return true 是网络 hook
 * @return false 不是网络 hook
 */
bool ConfigEbpf::isNetworkHook(HookType hook_type) noexcept {
  switch (hook_type) {
  case HookType::TC:
  case HookType::XDP_Generic:
    return true;
  default:
    return false;
  }
}

Configs::Configs(std::string_view configs_path) : Configs{} {
  try {
    *this = loadFromConfig(configs_path);
  } catch (const std::exception &exc) {
    LOG(warn, "Yaml configuration parse failed: {}", exc.what());
  }
}

Configs::Configs(const Configs &other) : Configs{} { *this = other; }

Configs &Configs::operator=(const Configs &other) {
  if (this != &other) {
    prometheus_enabled_ = other.prometheus_enabled_;
    prometheus_listen_ = other.prometheus_listen_;
    dbgserv_enabled_ = other.dbgserv_enabled_;
    dbgserv_addr_ = other.dbgserv_addr_;
    dbgserv_port_ = other.dbgserv_port_;
    loki_enabled_ = other.loki_enabled_;
    loki_host_ = other.loki_host_;
    loki_port_ = other.loki_port_;
    loki_path_ = other.loki_path_;
    loki_batch_size_ = other.loki_batch_size_;
    loki_flush_interval_ = other.loki_flush_interval_;
    ebpfs_ = other.ebpfs_;
  }
  return *this;
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
 * @brief 设置 loki 功能启用还是停用
 *
 * @param enabled 启用 true；停用 false
 */
void Configs::setLokiEnabled(bool enabled) { loki_enabled_ = enabled; }

/**
 * @brief 获取 loki 功能的启用停用状态
 *
 * @return true 启用
 * @return false 停用
 */
bool Configs::getLokiEnabled() const { return loki_enabled_; }

/**
 * @brief 设置 loki 服务端地址
 *
 * @param host 服务端地址
 */
void Configs::setLokiHost(std::string_view host) { loki_host_ = std::string{host}; }

/**
 * @brief 获取 loki 服务端地址
 *
 * @return std::string 服务端地址
 */
std::string Configs::getLokiHost() const { return loki_host_; }

/**
 * @brief 设置 loki 服务端端口号
 *
 * @param port 服务端端口号
 */
void Configs::setLokiPort(uint16_t port) { loki_port_ = port; }

/**
 * @brief 获取 loki 服务端端口号
 *
 * @return uint16_t 服务端端口号
 */
uint16_t Configs::getLokiPort() const { return loki_port_; }

/**
 * @brief 设置 loki 服务端资源路径
 *
 * @param path 服务端资源路径
 */
void Configs::setLokiPath(std::string_view path) { loki_path_ = path; }

/**
 * @brief 获取 loki 服务端地址
 *
 * @return std::string 服务端地址
 */
std::string Configs::getLokiPath() const { return loki_path_; }

/**
 * @brief 设置 loki 日志批处理数量
 *
 * @param size 批处理数量
 */
void Configs::setLokiBatchSize(int size) { loki_batch_size_ = size; }

/**
 * @brief 获取 loki 日志批处理数量
 *
 * @return int 数量
 */
int Configs::getLokiBatchSize() const { return loki_batch_size_; }

/**
 * @brief 获取 loki 日志刷新间隔
 *
 * @param seconds 时间间隔，单位秒
 */
void Configs::setLokiFlushInterval(int seconds) { loki_flush_interval_ = seconds; }

/**
 * @brief 获取 loki 日志刷新间隔
 *
 * @return int 时间间隔，单位秒
 */
int Configs::getLokiFlushInterval() const { return loki_flush_interval_; }

/**
 * @brief 设置 eBPF 全量配置
 *
 * @param ebpf_so eBPF 全量配置
 */
void Configs::setEbpfs(const EbpfMap &ebpf_so) { ebpfs_ = ebpf_so; }

/**
 * @brief 获取 eBPF 全量配置
 * @note 会拷贝一份副本返回外部
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
 * @param ifindex 网卡索引
 * @param order eBPF 调用链顺序
 * @param prog_id 唯一标识
 * @param prog_name eBPF 程序名称
 */
void Configs::appendEbpf(std::string_view name, std::string_view lib, std::string_view config,
                         int ifindex, int order, ProgId prog_id, std::string_view prog_name) {
  if (name.empty()) {
    return;
  }

  std::string name_str{name};

  auto iter = ebpfs_.find(name_str);
  if (iter != ebpfs_.end()) {
    iter->second = std::make_shared<ConfigEbpf>(lib, config, ifindex, order, prog_id, prog_name);
  } else {
    ebpfs_.emplace(name_str,
                   std::make_shared<ConfigEbpf>(lib, config, ifindex, order, prog_id, prog_name));
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
         dbgserv_port_ == other.dbgserv_port_ && loki_enabled_ == other.loki_enabled_ &&
         loki_host_ == other.loki_host_ && loki_port_ == other.loki_port_ &&
         loki_path_ == other.loki_path_ && loki_batch_size_ == other.loki_batch_size_ &&
         loki_flush_interval_ == other.loki_flush_interval_ && ebpfs_ == other.ebpfs_;
}

bool Configs::operator!=(const Configs &other) const { return !(*this == other); }

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

} // namespace daemon
} // namespace hebpf
