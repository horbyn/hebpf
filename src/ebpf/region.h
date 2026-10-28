#pragma once

// clang-format off
#include <mutex>
#include "nlohmann/json.hpp"
#include "src/event/event_holder.h"
#include "src/log/logger.h"
#include "region_map.h"
// clang-format on

namespace hebpf {
namespace ebpf {

class Region final : public event::EventBusHolder, public log::Loggable<log::Id::ebpf> {
public:
  explicit Region(std::weak_ptr<event::EventBusIf> event_bus);

  bool allocateRegion(const daemon::HookKey &hook_key);
  nlohmann::json getDebugStatus() const;

private:
  void onConfigEvent(const event::EventIf &event);
  void onHashFailureEvent(const event::EventIf &event);

  mutable std::mutex mutex_;
  std::shared_ptr<RegionMap> regions_;
};

} // namespace ebpf
} // namespace hebpf
