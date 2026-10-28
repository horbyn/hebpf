#pragma once

// clang-format off
#include <string_view>
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {
namespace services {
namespace runqlat {

constexpr std::string_view JKEY_RUNQCONF_LATENCY{"lantency_threshold"};

class RunqlatConf final {
public:
  void setLatency(uint64_t latency);
  uint64_t getLatency() const noexcept;

  friend void from_json(const nlohmann::json &json, RunqlatConf &conf);
  friend void to_json(nlohmann::json &json, const RunqlatConf &conf);

private:
  uint64_t latency_threshold_ns_;
};

} // namespace runqlat
} // namespace services
} // namespace hebpf
