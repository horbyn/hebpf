#pragma once

// clang-format off
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "src/daemon/hooks.h"
#include "src/event/event_holder.h"
#include "src/inotify/inotify_manager_if.h"
#include "src/io/io_if.h"
#include "src/log/logger.h"
#include "service_map.h"
// clang-format on

namespace hebpf {
namespace ebpf {

class EbpfManager final : public event::EventBusHolder, public log::Loggable<log::Id::ebpf> {
public:
  explicit EbpfManager(std::weak_ptr<event::EventBusIf> event_bus);
  ~EbpfManager();

  std::vector<EbpfIf *> getAllServices() const;
  nlohmann::json getDebugStatus() const;

  bool setIoContext(std::weak_ptr<io::IoIf> io_ctx);
  bool setInotifyManager(std::weak_ptr<inotify::InotifyManagerIf> inotify_manager);

private:
  bool applyDiff(std::vector<daemon::Diff> &diff, daemon::HookMap &result, bool rollback);
  EbpfIf *getService(const ServiceKey &hook) const;
  int loadService(const ServiceKey &hook, const daemon::ConfigEbpfPtr &config);
  bool unloadService(const ServiceKey &hook);
  bool registerServiceConfig(const ServiceKey &hook, const daemon::ConfigEbpfPtr &config);
  bool unregisterServiceConfig(const ServiceKey &hook);
  bool loadServiceConfig(const ServiceKey &hook);
  void onHashEvent(const event::EventIf &event);
  void onProgArrayFailureEvent(const event::EventIf &event);

  mutable std::mutex mutex_;
  std::weak_ptr<io::IoIf> io_ctx_;
  std::weak_ptr<inotify::InotifyManagerIf> inotify_mgr_;
  std::shared_ptr<ServiceMap> services_;
  std::vector<daemon::Diff> diff_;
};

} // namespace ebpf
} // namespace hebpf
