#pragma once

// clang-format off
#include "yaml-cpp/yaml.h"
#include "nlohmann/json.hpp"
#include "configs.hpp"
// clang-format on

namespace hebpf {
namespace daemon {

class Configurable {
public:
  virtual ~Configurable() = default;
  virtual void onConfigUpdate(HookType hook_type, int ifindex, const nlohmann::json &config) = 0;
};

} // namespace daemon
} // namespace hebpf
