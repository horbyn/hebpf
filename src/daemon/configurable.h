#pragma once

// clang-format off
#include "yaml-cpp/yaml.h"
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {
namespace daemon {

class Configurable {
public:
  virtual ~Configurable() = default;
  virtual void onConfigUpdate(const nlohmann::json &config) = 0;
};

} // namespace daemon
} // namespace hebpf
