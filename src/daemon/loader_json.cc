// clang-format off
#include "loader_json.h"
// clang-format on

namespace hebpf {
namespace daemon {

ServiceJson::ServiceJson(std::string_view so_path, std::string_view name,
                         std::string_view config_path, std::string_view hook, int ifindex,
                         const nlohmann::json &status) {
  setSoPath(so_path);
  setName(name);
  setConfigPath(config_path);
  setHook(hook);
  setIfindex(ifindex);
  setStatus(status);
}

/**
 * @brief 设置动态库路径
 *
 * @param so_path 动态库路径
 */
void ServiceJson::setSoPath(std::string_view so_path) { this->so_path = std::string{so_path}; }

/**
 * @brief 获取动态库路径
 *
 * @return std::string 动态库路径
 */
std::string ServiceJson::getSoPath() const { return so_path; }

/**
 * @brief 设置服务名称
 *
 * @param name 服务名称
 */
void ServiceJson::setName(std::string_view name) { this->name = std::string{name}; }

/**
 * @brief 获取服务名称
 *
 * @return std::string 服务名称
 */
std::string ServiceJson::getName() const { return name; }

/**
 * @brief 设置服务配置文件路径
 *
 * @param config_path 服务文件路径
 */
void ServiceJson::setConfigPath(std::string_view config_path) {
  this->config_path = std::string{config_path};
}

/**
 * @brief 获取服务配置文件路径
 *
 * @return std::string 服务配置文件路径
 */
std::string ServiceJson::getConfigPath() const { return config_path; }

/**
 * @brief 设置 hook
 *
 * @param hook 绑定点
 */
void ServiceJson::setHook(std::string_view hook) { this->hook = std::string{hook}; }

/**
 * @brief 获取 hook
 *
 * @return std::string 绑定点
 */
std::string ServiceJson::getHook() const { return hook; }

/**
 * @brief 设置网卡索引
 *
 * @param ifindex 网卡索引
 */
void ServiceJson::setIfindex(int ifindex) { this->ifindex = ifindex; }

/**
 * @brief 获取网卡索引
 *
 * @return int 网卡索引
 */
int ServiceJson::getIfindex() const { return ifindex; }

/**
 * @brief 设置服务状态
 *
 * @param status 服务状态
 */
void ServiceJson::setStatus(const nlohmann::json &status) { this->status = status; }

/**
 * @brief 获取服务状态
 *
 * @return nlohmann::json 服务状态
 */
nlohmann::json ServiceJson::getStatus() const { return status; }

LoaderJson::LoaderJson(const std::vector<ServiceJson> &services,
                       const std::vector<ebpf::EbpfProgMap> &pinned_map)
    : services{services}, pinned_map{pinned_map} {}

/**
 * @brief 追加服务
 *
 * @param service 服务
 */
void LoaderJson::appendService(const ServiceJson &service) { this->services.push_back(service); }

/**
 * @brief 追加 pinned map
 *
 * @param pinned_map 固定的 map
 */
void LoaderJson::appendPinnedMap(const ebpf::EbpfProgMap &pinned_map) {
  this->pinned_map.push_back(pinned_map);
}

} // namespace daemon
} // namespace hebpf
