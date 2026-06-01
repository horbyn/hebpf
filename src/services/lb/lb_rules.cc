// clang-format off
#include "lb_rules.h"
#include "spdlog/fmt/ranges.h"
#include "src/common/enum_name.hpp"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace services {
namespace lb {

Backends::Backends() : mac_{} {}

Backends::Backends(std::string_view mac) : Backends{} { setMac(mac); }

/**
 * @brief 设置 MAC 地址
 *
 * @param mac MAC 地址
 */
void Backends::setMac(std::string_view mac) { mac_ = std::string{mac}; }

/**
 * @brief 获取 MAC 地址
 *
 * @return std::string MAC 地址
 */
std::string Backends::getMac() const { return mac_; }

void from_json(const nlohmann::json &json, Backends &lb_rules_elem) {
  if (json.contains(JKEY_BACKENDS_MAC)) {
    lb_rules_elem.setMac(json.at(JKEY_BACKENDS_MAC).get<std::string>());
  }
}

void to_json(nlohmann::json &json, const Backends &lb_rules_elem) {
  json = nlohmann::json{{JKEY_BACKENDS_MAC, lb_rules_elem.getMac()}};
}

LbRulesElem::LbRulesElem() : vip_{}, vport_{}, backends_{} {}

LbRulesElem::LbRulesElem(std::string_view vip, uint16_t vport,
                         const std::vector<Backends> &backends)
    : LbRulesElem{} {
  setVip(vip);
  setVport(vport);
  setBackends(backends);
}

/**
 * @brief 设置虚拟 IP 地址
 *
 * @param vip 虚拟 IP 地址
 */
void LbRulesElem::setVip(std::string_view vip) { vip_ = std::string{vip}; }

/**
 * @brief 获取虚拟 IP 地址
 *
 * @return std::string 虚拟 IP 地址
 */
std::string LbRulesElem::getVip() const { return vip_; }

/**
 * @brief 设置虚拟端口
 *
 * @param vport 虚拟端口
 */
void LbRulesElem::setVport(uint16_t vport) { vport_ = vport; }

/**
 * @brief 获取虚拟端口
 *
 * @return uint16_t 虚拟端口
 */
uint16_t LbRulesElem::getVport() const noexcept { return vport_; }

/**
 * @brief 设置后端服务器列表
 *
 * @param backends 后端列表
 */
void LbRulesElem::setBackends(const std::vector<Backends> &backends) { backends_ = backends; }

/**
 * @brief 获取后端服务器列表
 *
 * @return std::vector<Backends> 后端列表
 */
std::vector<Backends> LbRulesElem::getBackends() const { return backends_; }

void from_json(const nlohmann::json &json, LbRulesElem &lb_rules_elem) {
  if (json.contains(JKEY_LBELEM_VIP)) {
    lb_rules_elem.setVip(json.at(JKEY_LBELEM_VIP).get<std::string>());
  }
  if (json.contains(JKEY_LBELEM_VPORT)) {
    lb_rules_elem.setVport(json.at(JKEY_LBELEM_VPORT).get<uint16_t>());
  }
  if (json.contains(JKEY_LBELEM_BACKENDS)) {
    lb_rules_elem.setBackends(json.at(JKEY_LBELEM_BACKENDS).get<std::vector<Backends>>());
  }
}

void to_json(nlohmann::json &json, const LbRulesElem &lb_rules_elem) {
  json = nlohmann::json{
      {JKEY_LBELEM_VIP, lb_rules_elem.getVip()},
      {JKEY_LBELEM_VPORT, lb_rules_elem.getVport()},
      {JKEY_LBELEM_BACKENDS, lb_rules_elem.getBackends()},
  };
}

LbRules::LbRules() : rules_{} {}

LbRules::LbRules(std::string_view local_mac, const std::vector<LbRulesElem> &rules) : LbRules{} {
  setLocalMac(local_mac);
  setRules(rules);
}

/**
 * @brief 设置本地 MAC 地址
 *
 * @param local_mac 地址
 */
void LbRules::setLocalMac(std::string_view local_mac) { local_mac_ = local_mac; }

/**
 * @brief 返回本地 MAC 地址
 *
 * @return std::string 本地 MAC 地址
 */
std::string LbRules::getLocalMac() const { return local_mac_; }

/**
 * @brief 设置 LB 规则
 *
 * @param rules 规则
 */
void LbRules::setRules(const std::vector<LbRulesElem> &rules) { rules_ = rules; }

/**
 * @brief 增加 LB 规则
 *
 * @param rule 规则
 */
void LbRules::addRules(const LbRulesElem &rule) { rules_.emplace_back(rule); }

/**
 * @brief 获取 LB 规则
 *
 * @return std::vector<LbRulesElem> 规则集合
 */
std::vector<LbRulesElem> LbRules::getRules() const { return rules_; }

/**
 * @brief 清空 LB 规则
 *
 */
void LbRules::clearRules() { rules_.clear(); }

void from_json(const nlohmann::json &json, LbRules &lb_rules) {
  if (json.contains(JKEY_LB_LOCAL_MAC)) {
    lb_rules.setLocalMac(json.at(JKEY_LB_LOCAL_MAC).get<std::string>());
  }
  if (json.contains(JKEY_LB_RULES)) {
    lb_rules.setRules(json.at(JKEY_LB_RULES).get<std::vector<LbRulesElem>>());
  }
}

void to_json(nlohmann::json &json, const LbRules &lb_rules) {
  json = nlohmann::json{{JKEY_LB_LOCAL_MAC, lb_rules.getLocalMac()},
                        {JKEY_LB_RULES, lb_rules.getRules()}};
}

} // namespace lb
} // namespace services
} // namespace hebpf
