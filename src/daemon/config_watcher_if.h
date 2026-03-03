#pragma once

// clang-format off
#include <string_view>
#include "src/daemon/configs.hpp"
#include "src/callback/callback.h"
// clang-format on

namespace hebpf {
namespace daemon {

using ConfigChangeCallback = common::Callback<void(const Configs &)>;

class ConfigWatcherIf {
public:
  virtual ~ConfigWatcherIf() = default;

  virtual void registerConfigChangeEvents(std::string_view config,
                                          ConfigChangeCallback callback) = 0;
};

} // namespace daemon
} // namespace hebpf
