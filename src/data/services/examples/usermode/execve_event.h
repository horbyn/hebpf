#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {
namespace services {
namespace examples {

constexpr std::string_view JKEY_USERMODE_NAME{"name"};
constexpr std::string_view JKEY_USERMODE_VALUE{"value"};
constexpr std::string_view JKEY_USERMODE_LABEL{"labels"};

class ExecveEvent final {
public:
  friend void from_json(const nlohmann::json &json, ExecveEvent &event);
  friend void to_json(nlohmann::json &json, const ExecveEvent &event);

  std::string name_;
  double value_;
  std::map<std::string, std::string> labels_;
};

} // namespace examples
} // namespace services
} // namespace hebpf
