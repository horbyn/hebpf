#pragma once

// clang-format off
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "src/event/event_holder.h"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace daemon {

class ConfigManager : public event::EventBusHolder, public log::Loggable<log::Id::daemon> {
public:
  explicit ConfigManager(std::weak_ptr<event::EventBusIf> event_bus);
  ~ConfigManager();

  void update(const Configs::EbpfMap &new_configs);
  nlohmann::json getDebugStatus() const;

private:
  struct Key {
    HookType hook;
    HookId hook_id;
    bool operator==(const Key &other) const {
      return hook == other.hook && hook_id == other.hook_id;
    }
  };
  struct KeyHash {
    size_t operator()(const Key &k) const {
      return std::hash<int>{}(static_cast<int>(k.hook)) ^ (std::hash<HookId>{}(k.hook_id) << 1);
    }
  };

  void cacheCurrent(const HookMap &hook_map);
  void onRegionFailure(const event::EventIf &event);
  void onComplete(const event::EventIf &event);
  std::shared_ptr<HookMap> computeDiff(const Configs::EbpfMap &new_configs,
                                       const std::shared_ptr<std::vector<Diff>> &results);
  std::shared_ptr<HookMap> translate(const Configs::EbpfMap &new_configs);
  ConfigEbpfPtr makeSchedulerEbpf(HookKey hook_key);

  mutable std::mutex mutex_;
  HookMap maps_;
};

} // namespace daemon
} // namespace hebpf
