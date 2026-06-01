#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include "nlohmann/json.hpp"
#include "src/daemon/configs.hpp"
#include "src/ebpf/ebpf_if.h"
#include "src/ebpf/pinned_prog_map.h"
// clang-format on

namespace hebpf {
namespace daemon {

class LoaderIf {
public:
  virtual ~LoaderIf() = default;
  virtual bool loadService(std::string_view so_path, HookType hook_type, int ifindex,
                           std::string_view config_path) = 0;
  virtual bool unloadServices(std::string_view so_path) = 0;
  virtual ebpf::EbpfIf *getService(std::string_view so_path) const = 0;
  virtual std::vector<std::string> getAllService() const = 0;
  virtual void registerServiceConfig(std::string_view so_path, HookType hook_type, int ifindex,
                                     std::string_view config_path) = 0;
  virtual void unregisterServiceConfig(std::string_view so_path, std::string_view config_path) = 0;
  virtual bool setProgChainManager(std::weak_ptr<ebpf::PinnedProgMap> mgr) = 0;
  virtual bool allocateChain(HookType hook_type, int ifindex) = 0;
  virtual bool updateProgChain(HookType hook_type,
                               const ebpf::PinnedProgMap::NamesIfindexVec &names_ifindex) = 0;
  virtual nlohmann::json getDebugStatus(void) = 0;
};

} // namespace daemon
} // namespace hebpf
