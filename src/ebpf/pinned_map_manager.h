#pragma once

// clang-format off
#include <memory>
#include <mutex>
#include "nlohmann/json.hpp"
#include "src/event/event_holder.h"
#include "src/log/logger.h"
#include "pinned_hash.h"
#include "pinned_prog_array.h"
#include "region.h"
// clang-format on

namespace hebpf {
namespace ebpf {

constexpr std::string_view JKEY_KERN_HASH{"hash"};
constexpr std::string_view JKEY_KERN_PRGOARR{"program_array"};

class PinnedMapManager : public event::EventBusHolder, public log::Loggable<log::Id::ebpf> {
public:
  explicit PinnedMapManager(std::weak_ptr<event::EventBusIf> event_bus);

  nlohmann::json getDebugStatus() const;

private:
  int getStartIndex(const RegionMap &region, const daemon::HookKey &hook_key) const;
  void onRegionEvent(const event::EventIf &event);
  void onEbpfEvent(const event::EventIf &event);
  void onEbpfFailureEvent(const event::EventIf &event);
  std::vector<std::string> getProgNames(const daemon::ProgList &prog_list) const;

  mutable std::mutex mutex_;
  std::unique_ptr<PinnedHash> pinned_hash_;
  std::unique_ptr<PinnedProgArray> pinned_prog_array_;
};

} // namespace ebpf
} // namespace hebpf
