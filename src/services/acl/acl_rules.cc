// clang-format off
#include "acl_rules.h"
#include "spdlog/fmt/ranges.h"
#include "src/common/enum_name.hpp"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace services {
namespace acl {

AclRulesElem::AclRulesElem()
    : saddr_{std::string{ACL_ANY_IPV4}}, daddr_{std::string{ACL_ANY_IPV4}}, sport_{ACL_ANY_PORT},
      dport_{ACL_ANY_PORT}, protocol_{AclRulesElem::Protocol::TCP},
      action_{AclRulesElem::Action::DROP} {}

AclRulesElem::AclRulesElem(std::string_view saddr, std::string_view daddr, uint16_t sport,
                           uint16_t dport, Protocol protocol, Action action)
    : saddr_{saddr}, daddr_{daddr}, sport_{sport}, dport_{dport}, protocol_{protocol},
      action_{action} {}

/**
 * @brief 设置源地址
 *
 * @param saddr 源地址
 */
void AclRulesElem::setSaddr(std::string_view saddr) { saddr_ = saddr; }

/**
 * @brief 获取源地址
 *
 * @return std::string 源地址
 */
std::string AclRulesElem::getSaddr() const { return saddr_; }

/**
 * @brief 设置目的地址
 *
 * @param daddr 目的地址
 */
void AclRulesElem::setDaddr(std::string_view daddr) { daddr_ = daddr; }

/**
 * @brief 获取目的地址
 *
 * @return std::string 目的地址
 */
std::string AclRulesElem::getDaddr() const { return daddr_; }

/**
 * @brief 设置源端口
 *
 * @param sport 源端口
 */
void AclRulesElem::setSport(uint16_t sport) { sport_ = sport; }

/**
 * @brief 获取源端口
 *
 * @return uint16_t 源端口
 */
uint16_t AclRulesElem::getSport() const { return sport_; }

/**
 * @brief 设置目的端口
 *
 * @param dport 目的端口
 */
void AclRulesElem::setDport(uint16_t dport) { dport_ = dport; }

/**
 * @brief 获取目的端口
 *
 * @return uint16_t 目的端口
 */
uint16_t AclRulesElem::getDport() const { return dport_; }

/**
 * @brief 设置协议
 *
 * @param protocol 协议
 */
void AclRulesElem::setProtocol(AclRulesElem::Protocol protocol) { protocol_ = protocol; }

/**
 * @brief 获取协议
 *
 * @return AclRules::Protocol 协议
 */
AclRulesElem::Protocol AclRulesElem::getProtocol() const { return protocol_; }

/**
 * @brief 设置动作
 *
 * @param action 动作
 */
void AclRulesElem::setAction(AclRulesElem::Action action) { action_ = action; }

/**
 * @brief 获取动作
 *
 * @return AclRulesElem::Action 动作
 */
AclRulesElem::Action AclRulesElem::getAction() const { return action_; }

/**
 * @brief 字符串转换为协议
 *
 * @param protocol 协议字符串
 * @return AclRulesElem::Protocol 协议枚举
 */
AclRulesElem::Protocol AclRulesElem::stringToProtocol(std::string_view protocol) {
  if (protocol == "tcp" || protocol == "TCP") {
    return Protocol::TCP;
  } else if (protocol == "udp" || protocol == "UDP") {
    return Protocol::UDP;
  } else {
    return Protocol::UNKNOWN;
  }
}

/**
 * @brief 协议枚举转换字符串
 *
 * @param protocol 协议枚举
 * @return std::string 字符串
 */
std::string AclRulesElem::ProtocolToString(Protocol protocol) {
  switch (protocol) {
  case Protocol::TCP:
    return "TCP";
  case Protocol::UDP:
    return "UDP";
  default:
    return "UNKNOWN";
  }
}

/**
 * @brief 字符串转换为动作
 *
 * @param action 动作字符串
 * @return AclRulesElem::Action 动作枚举
 */
AclRulesElem::Action AclRulesElem::stringToAction(std::string_view action) {
  if (action == "accept" || action == "ACCEPT") {
    return Action::ACCEPT;
  } else if (action == "drop" || action == "DROP") {
    return Action::DROP;
  } else {
    return Action::UNKNOWN;
  }
}

/**
 * @brief 动作枚举转换字符串
 *
 * @param action 动作枚举
 * @return std::string 字符串
 */
std::string AclRulesElem::ActionToString(Action action) {
  switch (action) {
  case Action::ACCEPT:
    return "ACCEPT";
  case Action::DROP:
    return "DROP";
  default:
    return "UNKNOWN";
  }
}

void from_json(const nlohmann::json &json, AclRulesElem &acl) {
  if (json.contains(JKEY_ACL_SADDR)) {
    acl.setSaddr(json.at(JKEY_ACL_SADDR).get<std::string>());
  }
  if (json.contains(JKEY_ACL_DADDR)) {
    acl.setDaddr(json.at(JKEY_ACL_DADDR).get<std::string>());
  }
  if (json.contains(JKEY_ACL_SPORT)) {
    acl.setSport(json.at(JKEY_ACL_SPORT).get<uint16_t>());
  }
  if (json.contains(JKEY_ACL_DPORT)) {
    acl.setDport(json.at(JKEY_ACL_DPORT).get<uint16_t>());
  }
  if (json.contains(JKEY_ACL_PROTOCOL)) {
    auto proto_opt =
        stringEnum<AclRulesElem::Protocol>(json.at(JKEY_ACL_PROTOCOL).get<std::string>());
    if (proto_opt) {
      acl.setProtocol(*proto_opt);
    } else {
      auto vec = enumNameList<AclRulesElem::Protocol>();
      vec.pop_back(); // UNKNOWN
      std::string desc = fmt::format("{}", fmt::join(vec, "/"));
      GLOBAL_LOG(warn, "{} configuration error: unknown protocol \"{}\", expected {}",
                 SERVICE_NAME_ACL, json.at(JKEY_ACL_PROTOCOL).get<std::string>(), desc);
      acl.setProtocol(AclRulesElem::Protocol::UNKNOWN);
    }
  }
  if (json.contains(JKEY_ACL_ACTION)) {
    auto action_opt = stringEnum<AclRulesElem::Action>(json.at(JKEY_ACL_ACTION).get<std::string>());
    if (action_opt) {
      acl.setAction(*action_opt);
    } else {
      auto vec = enumNameList<AclRulesElem::Action>();
      vec.pop_back(); // UNKNOWN
      std::string desc = fmt::format("{}", fmt::join(vec, "/"));
      GLOBAL_LOG(warn, "{} configuration error: unknown action \"{}\", expected {}",
                 SERVICE_NAME_ACL, json.at(JKEY_ACL_ACTION).get<std::string>(), desc);
      acl.setAction(AclRulesElem::Action::UNKNOWN);
    }
  }
}

void to_json(nlohmann::json &json, const AclRulesElem &acl) {
  json = nlohmann::json{{JKEY_ACL_SADDR, acl.getSaddr()},
                        {JKEY_ACL_DADDR, acl.getDaddr()},
                        {JKEY_ACL_SPORT, acl.getSport()},
                        {JKEY_ACL_DPORT, acl.getDport()},
                        {JKEY_ACL_PROTOCOL, std::string{enumName(acl.getProtocol())}},
                        {JKEY_ACL_ACTION, std::string{enumName(acl.getAction())}}};
}

AclRules::AclRules() : ifindex_{INVALID_IFINDEX}, rules_{} {}

AclRules::AclRules(HookType hook, int ifindex, const std::vector<AclRulesElem> &rules)
    : hook_{hook}, ifindex_{ifindex}, rules_{rules} {}

void AclRules::setHook(HookType hook) { hook_ = hook; }

AclRules::HookType AclRules::getHook() const noexcept { return hook_; }

/**
 * @brief 设置网卡索引
 *
 * @param ifindex 网卡索引
 */
void AclRules::setIfindex(int ifindex) { ifindex_ = ifindex; }

/**
 * @brief 获取网卡索引
 *
 * @return int 网卡索引
 */
int AclRules::getIfindex() const noexcept { return ifindex_; }

/**
 * @brief 设置 ACL 规则
 *
 * @param rules 规则集
 */
void AclRules::setRules(const std::vector<AclRulesElem> &rules) { rules_ = rules; }

/**
 * @brief 增加一条 ACL 规则
 *
 * @param rule 规则
 */
void AclRules::addRules(const AclRulesElem &rule) { rules_.emplace_back(rule); }

/**
 * @brief 获取 ACL 规则集
 *
 * @return std::vector<AclRulesElem> 规则集
 */
std::vector<AclRulesElem> AclRules::getRules() const { return rules_; }

/**
 * @brief 清空所有 ACL 规则
 *
 */
void AclRules::clearRules() { rules_.clear(); }

void from_json(const nlohmann::json &json, AclRules &acl) {
  if (json.contains(JKEY_ACL_HOOK)) {
    auto hook_opt = stringEnum<AclRules::HookType>(json.at(JKEY_ACL_HOOK).get<std::string>());
    if (hook_opt) {
      acl.setHook(*hook_opt);
    } else {
      auto vec = enumNameList<AclRules::HookType>();
      vec.pop_back(); // UNKNOWN
      std::string desc = fmt::format("{}", fmt::join(vec, "/"));
      GLOBAL_LOG(warn, "{} configuration error: unknown hook \"{}\", expected {}", SERVICE_NAME_ACL,
                 json.at(JKEY_ACL_HOOK).get<std::string>(), desc);
      acl.setHook(AclRules::HookType::UNKNOWN);
    }
  }
  if (json.contains(JKEY_ACL_IFINDEX)) {
    acl.setIfindex(json.at(JKEY_ACL_IFINDEX).get<int>());
  }
  if (json.contains(JKEY_ACL_RULES)) {
    acl.setRules(json.at(JKEY_ACL_RULES).get<std::vector<AclRulesElem>>());
  }
}

void to_json(nlohmann::json &json, const AclRules &acl) {
  json = nlohmann::json{{JKEY_ACL_IFINDEX, acl.getIfindex()},
                        {JKEY_ACL_RULES, acl.getRules()},
                        {JKEY_ACL_HOOK, std::string{enumName(acl.getHook())}}};
}

} // namespace acl
} // namespace services
} // namespace hebpf
