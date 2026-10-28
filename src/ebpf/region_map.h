#pragma once

// clang-format off
#include <string_view>
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "src/daemon/hooks.h"
// clang-format on

namespace hebpf {
namespace ebpf {

using RegionMap = std::unordered_map<daemon::HookKey, int, daemon::HookKeyHash>;

} // namespace ebpf
} // namespace hebpf
