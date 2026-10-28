// clang-format off
#include "daemon_json.h"
// clang-format on

namespace hebpf {
namespace daemon {

/**
 * @brief 有参构造
 *
 * @param running 是否运行
 * @param queue_empty 队列是否为空
 * @param queue_full 队列是否满
 * @param conf 配置管理器中所有配置
 * @param regions Region 中所有配置
 * @param services 服务管理器中所有服务
 * @param pinned_maps 固定表中所有配置
 */
DaemonJson::DaemonJson(bool running, bool queue_empty, bool queue_full, const nlohmann::json &conf,
                       const nlohmann::json &regions, const nlohmann::json &services,
                       const nlohmann::json &pinned_maps) {
  setRunning(running);
  setQueueEmpty(queue_empty);
  setQueueFull(queue_full);
  setConfigurations(conf);
  setRegions(regions);
  setServices(services);
  setPinnedMaps(pinned_maps);
}

/**
 * @brief 设置运行标识
 *
 * @param running 运行标识，true 正在运行；false 停止
 */
void DaemonJson::setRunning(bool running) { running_ = running; }

/**
 * @brief 获取运行标识
 *
 * @return true 正在运行
 * @return false 停止
 */
bool DaemonJson::getRunning() const { return running_; }

/**
 * @brief 设置队列空标识
 *
 * @param queue_empty 队列空标识，true 队列空；false 队列非空
 */
void DaemonJson::setQueueEmpty(bool queue_empty) { queue_empty_ = queue_empty; }

/**
 * @brief 获取队列空标识
 *
 * @return true 队列空
 * @return false 队列非空
 */
bool DaemonJson::getQueueEmpty() const { return queue_empty_; }

/**
 * @brief 设置队列满标识
 *
 * @param queue_full 队列满标识，true 队列满；false 队列非满
 */
void DaemonJson::setQueueFull(bool queue_full) { queue_full_ = queue_full; }

/**
 * @brief 获取队列满标识
 *
 * @return true 队列满
 * @return false 队列非满
 */
bool DaemonJson::getQueueFull() const { return queue_full_; }

/**
 * @brief 设置配置对象
 *
 * @param conf 配置对象
 */
void DaemonJson::setConfigurations(const nlohmann::json &conf) { configurations_ = conf; }

/**
 * @brief 获取配置对象
 *
 * @return nlohmann::json 配置对象
 */
nlohmann::json DaemonJson::getConfigurations() const { return configurations_; }

/**
 * @brief 设置 region 对象
 *
 * @param region region 对象
 */
void DaemonJson::setRegions(const nlohmann::json &region) { regions_ = region; }

/**
 * @brief 获取 region 对象
 *
 * @return nlohmann::json region 对象
 */
nlohmann::json DaemonJson::getRegions() const { return regions_; }

/**
 * @brief 设置服务对象
 *
 * @param services 服务对象
 */
void DaemonJson::setServices(const nlohmann::json &services) { services_ = services; }

/**
 * @brief 获取服务对象
 *
 * @return nlohmann::json 服务对象
 */
nlohmann::json DaemonJson::getServices() const { return services_; }

/**
 * @brief 设置固定表对象
 *
 * @param pinned 固定表对象
 */
void DaemonJson::setPinnedMaps(const nlohmann::json &pinned) { pinned_maps_ = pinned; }

/**
 * @brief 获取固定表对象
 *
 * @return nlohmann::json 固定表对象
 */
nlohmann::json DaemonJson::getPinnedMaps() const { return pinned_maps_; }

void to_json(nlohmann::json &json, const DaemonJson &daemon) {
  json = nlohmann::json{
      {JKEY_DAEMONJSON_RUN, daemon.running_},       {JKEY_DAEMONJSON_QEMPTY, daemon.queue_empty_},
      {JKEY_DAEMONJSON_QFULL, daemon.queue_full_},  {JKEY_DAEMONJSON_CONF, daemon.configurations_},
      {JKEY_DAEMONJSON_REGION, daemon.regions_},    {JKEY_DAEMONJSON_SERV, daemon.services_},
      {JKEY_DAEMONJSON_PINNED, daemon.pinned_maps_}};
}

} // namespace daemon
} // namespace hebpf
