#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {

constexpr std::string_view JKEY_METRICS_SERVICE{"service"};
constexpr std::string_view JKEY_METRICS_METRICS{"metrics"};

class Metrics final {
public:
  friend void from_json(const nlohmann::json &json, Metrics &metrics);
  friend void to_json(nlohmann::json &json, const Metrics &metrics);

  std::string service_;
  std::vector<nlohmann::json> metrics_;
};

} // namespace hebpf
