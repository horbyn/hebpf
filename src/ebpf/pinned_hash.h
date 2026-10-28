#pragma once

// clang-format off
#include <unordered_set>
#include "pinned_map_if.h"
#include "src/daemon/hooks.h"
// clang-format on

namespace hebpf {
namespace ebpf {

// constexpr std::string_view

class PinnedHash final : public PinnedMapBase {
public:
  bool init() override;
  nlohmann::json getDebugStatus() override;

  bool updateMapElem(const daemon::HookKey &hook_key, int start_index, daemon::ProgList progs);
  bool cleanMap(daemon::HookKey hook_key);
  bool rollback(const daemon::HookMap &current_map);

private:
  bpf_map_type getMapType() const override;
  size_t getKeySize() const override;
  size_t getValueSize() const override;

  daemon::HookMap getDiff(const daemon::HookMap &current_map) const;

  daemon::HookMap cached_;
};

} // namespace ebpf
} // namespace hebpf
