// clang-format off
#include "config_map.h"
#include <bpf/bpf.h>
#include <unistd.h>
#include "src/common/assert.h"
#include "src/common/exception.h"
// clang-format on

namespace hebpf {
namespace ebpf {

ConfigMap::ConfigMap(std::string_view map_name, size_t max_entries)
    : map_name_(map_name), map_size_(max_entries) {}

/**
 * @brief 初始化公共 map，若已存在则打开，否则创建并 pin
 *
 * @return true 成功
 * @return false 失败
 */
bool ConfigMap::init() {
  if (initialized_.load()) {
    return true;
  }

  ASSERT(!map_name_.empty());
  ASSERT(map_size_ > 0);

  if (!createPinnedInfo(map_name_, map_size_)) {
    return false;
  }

  initialized_.store(true);
  return true;
}

/**
 * @brief 获取 eBPF 配置 Map 的状态信息对接 Debug 服务器
 *
 * @return nlohmann::json JSON 对象
 */
nlohmann::json ConfigMap::getDebugStatus() { throw EXCEPT("not implemented"); }

/**
 * @brief 设置配置项的值
 *
 * @param key 配置项下标
 * @param value 值
 * @return true 成功
 * @return false 失败
 */
bool ConfigMap::setConfig(__u32 key, __u64 value) {
  if (!initialized_.load()) {
    return false;
  }

  if (bpf_map_update_elem(getPinnedFd(), &key, &value, BPF_ANY) != 0) {
    LOG(error, "Failed to update config map {} at key {}: {}", map_name_, key, strerror(errno));
    return false;
  }
  return true;
}

/**
 * @brief 获取配置项的值
 *
 * @param key 配置项下标
 * @param value 输出参数
 * @return true 成功
 * @return false 失败
 */
bool ConfigMap::getConfig(__u32 key, __u64 &value) const {
  if (!initialized_.load()) {
    return false;
  }

  if (bpf_map_lookup_elem(getPinnedFd(), &key, &value) != 0) {
    LOG(warn, "Failed to lookup config map {} at key {}: {}", map_name_, key, strerror(errno));
    return false;
  }
  return true;
}

/**
 * @brief 获取 map 类型
 *
 * @return bpf_map_type 类型
 */
bpf_map_type ConfigMap::getMapType() const { return BPF_MAP_TYPE_ARRAY; }

/**
 * @brief 获取 key 大小
 *
 * @return size_t 大小
 */
size_t ConfigMap::getKeySize() const { return sizeof(__u32); }

/**
 * @brief 获取 value 大小
 *
 * @return size_t 大小
 */
size_t ConfigMap::getValueSize() const { return sizeof(__u64); }

} // namespace ebpf
} // namespace hebpf
