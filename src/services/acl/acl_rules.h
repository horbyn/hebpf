#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {
namespace services {
namespace acl {

constexpr std::string_view SERVICE_NAME_ACL{"acl"};
constexpr std::string_view ACL_ANY_IPV4{"0.0.0.0"};
constexpr uint16_t ACL_ANY_PORT{0};
constexpr int INVALID_IFINDEX{-1};

constexpr std::string_view JKEY_ACL_SADDR{"saddr"};
constexpr std::string_view JKEY_ACL_DADDR{"daddr"};
constexpr std::string_view JKEY_ACL_SPORT{"sport"};
constexpr std::string_view JKEY_ACL_DPORT{"dport"};
constexpr std::string_view JKEY_ACL_PROTOCOL{"protocol"};
constexpr std::string_view JKEY_ACL_ACTION{"action"};
constexpr std::string_view JKEY_ACL_HOOK{"hook"};
constexpr std::string_view JKEY_ACL_IFINDEX{"ifindex"};
constexpr std::string_view JKEY_ACL_RULES{"rules"};

class AclRulesElem final {
public:
  enum class Protocol : uint8_t { TCP, UDP, UNKNOWN };
  enum class Action : uint8_t { DROP, ACCEPT, UNKNOWN };

  explicit AclRulesElem();
  explicit AclRulesElem(std::string_view saddr, std::string_view daddr, uint16_t sport,
                        uint16_t dport, Protocol protocol, Action action);

  void setSaddr(std::string_view saddr);
  std::string getSaddr() const;

  void setDaddr(std::string_view daddr);
  std::string getDaddr() const;

  void setSport(uint16_t sport);
  uint16_t getSport() const;

  void setDport(uint16_t dport);
  uint16_t getDport() const;

  void setProtocol(Protocol protocol);
  Protocol getProtocol() const;

  void setAction(Action action);
  Action getAction() const;

  static Protocol stringToProtocol(std::string_view protocol);
  static std::string ProtocolToString(Protocol protocol);
  static Action stringToAction(std::string_view action);
  static std::string ActionToString(Action action);

  friend void from_json(const nlohmann::json &json, AclRulesElem &acl);
  friend void to_json(nlohmann::json &json, const AclRulesElem &acl);

private:
  std::string saddr_;
  std::string daddr_;
  uint16_t sport_;
  uint16_t dport_;
  Protocol protocol_;
  Action action_;
};

class AclRules final {
public:
  enum class HookType : uint8_t { TC, XDP_GENERIC, XDP_NATIVE, XDP_OFFLOAD, UNKNOWN };

  explicit AclRules();
  explicit AclRules(HookType hook, int ifindex, const std::vector<AclRulesElem> &rules);

  void setHook(HookType hook);
  HookType getHook() const noexcept;

  void setIfindex(int ifindex);
  int getIfindex() const noexcept;

  void setRules(const std::vector<AclRulesElem> &rules);
  void addRules(const AclRulesElem &rule);
  std::vector<AclRulesElem> getRules() const;
  void clearRules();

  friend void from_json(const nlohmann::json &json, AclRules &acl);
  friend void to_json(nlohmann::json &json, const AclRules &acl);

private:
  HookType hook_;
  int ifindex_;
  std::vector<AclRulesElem> rules_;
};

} // namespace acl
} // namespace services
} // namespace hebpf
