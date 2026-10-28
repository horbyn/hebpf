// clang-format off
#include "hooks.h"
#include "src/common/assert.h"
// clang-format on

namespace hebpf {
namespace daemon {

bool HookKey::operator==(const HookKey &other) const {
  return hook == other.hook && hook_id == other.hook_id;
}

size_t HookKeyHash::operator()(const HookKey &key) const {
  return std::hash<int>{}(static_cast<int>(key.hook)) ^ (std::hash<HookId>{}(key.hook_id) << 1);
}

Diff::Diff(DiffType type, HookKey hook, ConfigEbpfPtr config)
    : type_{type}, hook_{hook}, config_{std::move(config)} {}

/**
 * @brief 获取类型
 *
 * @return DiffType 类型
 */
DiffType Diff::getType() const noexcept { return type_; }

/**
 * @brief 获取绑定类型
 *
 * @return HookKey 绑定类型
 */
HookKey Diff::getHook() const { return hook_; }

/**
 * @brief 获取配置对象
 *
 * @return ConfigEbpfPtr 配置对象
 */
ConfigEbpfPtr Diff::getConfig() const { return config_; }

/**
 * @brief 从 diff 中移除所有 "未在 map 中出现" 的条目
 *
 * @param map_ptr 参考的 HookMap（不修改）
 * @param diff_ptr 待过滤的 Diff 列表（原地修改）
 */
void Diff::filterByMap(std::shared_ptr<HookMap> map_ptr,
                       std::shared_ptr<std::vector<Diff>> diff_ptr) {
  auto &diff = *diff_ptr;
  auto &map = *map_ptr;

  auto newEnd = std::remove_if(diff.begin(), diff.end(), [&map](const Diff &diff_elem) {
    auto iter = map.find(diff_elem.getHook());
    if (iter == map.end()) {
      return true; // map 里没有这个 hook，标记为删除
    }

    auto config = diff_elem.getConfig();
    ASSERT(config != nullptr);
    // map 中出现了这个 hook，但对应的 ProgList 中没有这个配置，也标记为删除
    auto it = std::find_if(iter->second.begin(), iter->second.end(),
                           [&config](const ConfigEbpfPtr &conf_elem) {
                             return conf_elem->getName() == config->getName();
                           });
    return it == iter->second.end();
  });
  diff.erase(newEnd, diff.end());
}

/**
 * @brief 从 diff 中移除所有 "在 other 中出现" 的条目
 *
 * @param result 结果（原地修改）
 * @param other 其他
 */
void Diff::subtract(std::vector<Diff> &result, const std::vector<Diff> &other) {
  for (auto iter = result.begin(); iter != result.end();) {
    if (std::find_if(other.begin(), other.end(), [self = *iter](const Diff &other_elem) {
          return other_elem.getHook() == self.getHook() &&
                 other_elem.getConfig()->getName() == self.getConfig()->getName();
        }) != other.end()) {
      iter = result.erase(iter);
    } else {
      ++iter;
    }
  }
}

} // namespace daemon
} // namespace hebpf
