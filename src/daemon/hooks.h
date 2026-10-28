#pragma once

// clang-format off
#include <memory>
#include "configs.hpp"
// clang-format on

namespace hebpf {
namespace daemon {

constexpr std::string_view JKEY_HOOKKEY_HOOK{"hook"};
constexpr std::string_view JKEY_HOOKKEY_HOOKID{"hook_id"};
constexpr std::string_view JKEY_HOOKMAP{"hook_map"};

/**
 * @brief 一个 hook 是根据 hook 类型和 hook id 共同来决定的
 */
struct HookKey {
  HookType hook;
  HookId hook_id;
  bool operator==(const HookKey &other) const;
};
void from_json(const nlohmann::json &json, HookKey &hook_key);
void to_json(nlohmann::json &json, const HookKey &hook_key);

/**
 * @brief HookKey 哈希函数
 *
 */
struct HookKeyHash {
  size_t operator()(const HookKey &key) const;
};

/**
 * @brief 配置表 - 程序列表
 *
 */
using ProgList = std::vector<ConfigEbpfPtr>;
void to_json(nlohmann::json &json, const ProgList &conf_ptr);

/**
 * @brief 配置表
 *
 */
using HookMap = std::unordered_map<HookKey, ProgList, HookKeyHash>;
void to_json(nlohmann::json &json, const HookMap &map);

enum class DiffType : uint8_t { ADD, REMOVE, KEEP };

/**
 * @brief 配置差异
 *
 */
class Diff {
public:
  explicit Diff(DiffType type, HookKey hook, ConfigEbpfPtr config);

  DiffType getType() const noexcept;
  HookKey getHook() const;
  ConfigEbpfPtr getConfig() const;

  static void filterByMap(std::shared_ptr<HookMap> map, std::shared_ptr<std::vector<Diff>> diff);
  static void subtract(std::vector<Diff> &result, const std::vector<Diff> &other);

private:
  DiffType type_;
  HookKey hook_;
  ConfigEbpfPtr config_;
};

} // namespace daemon
} // namespace hebpf
