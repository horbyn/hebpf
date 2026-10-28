// clang-format off
#include "hooks.h"
#include "src/common/enum_name.hpp"
// clang-format on

namespace hebpf {
namespace daemon {

/**
 * @brief json 对象转换 HookKey 对象
 *
 * @param json json 对象
 * @param hook_key HookKey 对象
 */
void from_json(const nlohmann::json &json, HookKey &hook_key) {
  if (json.contains(JKEY_HOOKKEY_HOOK)) {
    auto hook_opt = stringEnum<HookType>(json.at(JKEY_HOOKKEY_HOOK).get<std::string>());
    hook_key.hook = hook_opt.value_or(HookType::UNKNOWN);
  }
  if (json.contains(JKEY_HOOKKEY_HOOKID)) {
    hook_key.hook_id = json.at(JKEY_HOOKKEY_HOOKID).get<HookId>();
  }
}

/**
 * @brief HookKey 对象转换 json 对象
 *
 * @param json json 对象
 * @param hook_key HookKey 对象
 */
void to_json(nlohmann::json &json, const HookKey &hook_key) {
  json = nlohmann::json{{JKEY_HOOKKEY_HOOK, enumName(hook_key.hook)},
                        {JKEY_HOOKKEY_HOOKID, hook_key.hook_id}};
}

/**
 * @brief json 对象转换 ConfigEbpf 对象
 *
 * @param json json 对象
 * @param config ConfigEbpf 对象
 */
void from_json(const nlohmann::json &json, ConfigEbpf &config) {
  if (json.contains(CONFIGS_EBPF_LIB)) {
    config.setLib(json.at(CONFIGS_EBPF_LIB).get<std::string>());
  }
  if (json.contains(CONFIGS_EBPF_CONFIG)) {
    config.setConfig(json.at(CONFIGS_EBPF_CONFIG).get<std::string>());
  }
  if (json.contains(CONFIGS_EBPF_IFINDEX)) {
    config.setIfindex(json.at(CONFIGS_EBPF_IFINDEX).get<int>());
  }
  if (json.contains(CONFIGS_EBPF_ORDER)) {
    config.setOrder(json.at(CONFIGS_EBPF_ORDER).get<int>());
  }
  if (json.contains(CONFIGS_EBPF_PROGID)) {
    config.setProgId(json.at(CONFIGS_EBPF_PROGID).get<ProgId>());
  }
  if (json.contains(CONFIGS_EBPF_NAME)) {
    config.setName(json.at(CONFIGS_EBPF_NAME).get<std::string>());
  }
}

/**
 * @brief ConfigEbpf 对象转换 json 对象
 *
 * @param json json 对象
 * @param config ConfigEbpf 对象
 */
void to_json(nlohmann::json &json, const ConfigEbpf &config) {
  json = nlohmann::json{
      {CONFIGS_EBPF_LIB, config.getLib()},         {CONFIGS_EBPF_CONFIG, config.getConfig()},
      {CONFIGS_EBPF_IFINDEX, config.getIfindex()}, {CONFIGS_EBPF_ORDER, config.getOrder()},
      {CONFIGS_EBPF_PROGID, config.getProgId()},   {CONFIGS_EBPF_NAME, config.getName()}};
}

/**
 * @brief ProgList 对象转换 json 对象
 *
 * @param json json 对象
 * @param conf_ptr ProgList 对象
 */
void to_json(nlohmann::json &json, const ProgList &conf_ptr) {
  json = nlohmann::json::array();
  for (const auto &ptr : conf_ptr) {
    json.emplace_back(*ptr);
  }
}

/**
 * @brief HookMap 对象转换 json 对象
 *
 * @param json json 对象
 * @param map HookMap 对象
 */
void to_json(nlohmann::json &json, const HookMap &map) {
  auto array = nlohmann::json::array();
  for (const auto &[hook_key, prog_list] : map) {
    array.emplace_back(nlohmann::json{{"key", hook_key}, {"value", prog_list}});
  }
  json = nlohmann::json{{JKEY_HOOKMAP, array}};
}

} // namespace daemon
} // namespace hebpf
