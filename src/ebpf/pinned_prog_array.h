#pragma once

// clang-format off
#include "pinned_map_if.h"
#include "pinned_hash.h"
#include "service_map.h"
// clang-format on

namespace hebpf {
namespace ebpf {

class PinnedProgArray final : public PinnedMapBase {
public:
  bool init() override;
  nlohmann::json getDebugStatus() override;

  bool updateMap(daemon::HookMap &map, const PinnedHash *hash_obj,
                 std::shared_ptr<ServiceMap> service_map);

private:
  bpf_map_type getMapType() const override;
  size_t getKeySize() const override;
  size_t getValueSize() const override;

  int getServiceFd(std::shared_ptr<ServiceMap> service_map, const daemon::HookKey &hook_key,
                   std::string_view name) const;

  bool updateMapElem(const daemon::HookKey &hook_key, const daemon::ConfigEbpfPtr &progs,
                     int hash_fd, int prog_running_fd);
};

} // namespace ebpf
} // namespace hebpf
