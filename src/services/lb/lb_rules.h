#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {
namespace services {
namespace lb {

constexpr std::string_view JKEY_BACKENDS_MAC{"mac"};
constexpr std::string_view JKEY_LBELEM_VIP{"vip"};
constexpr std::string_view JKEY_LBELEM_VPORT{"vport"};
constexpr std::string_view JKEY_LBELEM_BACKENDS{"backends"};
constexpr std::string_view JKEY_LB_LOCAL_MAC{"local_mac"};
constexpr std::string_view JKEY_LB_RULES{"rules"};

constexpr std::string_view SERVICE_NAME_LB{"lb"};

class Backends final {
public:
  explicit Backends();
  explicit Backends(std::string_view mac);

  void setMac(std::string_view mac);
  std::string getMac() const;

  friend void from_json(const nlohmann::json &json, Backends &lb_rules_elem);
  friend void to_json(nlohmann::json &json, const Backends &lb_rules_elem);

private:
  std::string mac_;
};

class LbRulesElem final {
public:
  explicit LbRulesElem();
  explicit LbRulesElem(std::string_view vip, uint16_t vport, const std::vector<Backends> &backends);

  void setVip(std::string_view vip);
  std::string getVip() const;

  void setVport(uint16_t vport);
  uint16_t getVport() const noexcept;

  void setBackends(const std::vector<Backends> &backends);
  std::vector<Backends> getBackends() const;

  friend void from_json(const nlohmann::json &json, LbRulesElem &lb_rules_elem);
  friend void to_json(nlohmann::json &json, const LbRulesElem &lb_rules_elem);

private:
  std::string vip_;
  uint16_t vport_;
  std::vector<Backends> backends_;
};

class LbRules final {
public:
  explicit LbRules();
  explicit LbRules(std::string_view local_mac, const std::vector<LbRulesElem> &rules);

  void setLocalMac(std::string_view local_mac);
  std::string getLocalMac() const;

  void setRules(const std::vector<LbRulesElem> &rules);
  void addRules(const LbRulesElem &rule);
  std::vector<LbRulesElem> getRules() const;
  void clearRules();

  friend void from_json(const nlohmann::json &json, LbRules &lb_rules);
  friend void to_json(nlohmann::json &json, const LbRules &lb_rules);

private:
  std::string local_mac_;
  std::vector<LbRulesElem> rules_;
};

} // namespace lb
} // namespace services
} // namespace hebpf
