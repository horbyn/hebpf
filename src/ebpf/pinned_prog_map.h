#pragma once

// clang-format off
#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "pinned_map_if.h"
#include "generated/ebpf_prog_map.h"
// clang-format on

namespace hebpf {
namespace ebpf {

// TODO: Scheduler 动态库路径需要更灵活给出
constexpr std::string_view SCHEDULER_XDP_LIB{
    "build/src/services/scheduler/libhebpf_scheduler_xdp.so"};
constexpr std::string_view SCHEDULER_TC_LIB{"build/src/services/scheduler/libhebpf_scheduler.so"};

/**
 * @brief 管理尾调用链的公共 eBPF map
 *
 * 支持 TC_Ingress 和 XDP_Generic 两种 hook 类型。
 * map 会 pin 到 BPF 文件系统，确保不同 eBPF 程序共享。
 */
class PinnedProgMap : public PinnedMapIf {
public:
  explicit PinnedProgMap() = default;
  ~PinnedProgMap();

  PinnedProgMap(const PinnedProgMap &) = delete;
  PinnedProgMap &operator=(const PinnedProgMap &) = delete;

  bool init() override;
  void cleanup() override;
  nlohmann::json getDebugStatus() override;

  int allocateRegion(daemon::HookType hook_type, int ifindex);
  void releaseRegion(daemon::HookType hook_type, int ifindex);
  int getStartIndex(daemon::HookType hook_type, int ifindex) const;

  using NamesIfindexVec = std::vector<std::pair<std::string, int>>;
  bool updateChain(daemon::HookType hook_type, const NamesIfindexVec &ordered_names);
  bool insertProg(daemon::HookType hook_type, std::string_view prog_name, int prog_fd, int ifindex);

  bool lookupProgInfo(std::string_view name, int &id, daemon::HookType &hook) const;

  enum class UnofficialName : uint8_t { PROGRAM /* 程序名称 */, LIBRARY /* 动态库名称 */ };
  static std::string getCanonicalName(daemon::HookType hook_type, UnofficialName type,
                                      std::string_view name);

private:
  struct HookMaps {
    int prog_array_fd{-1};
    int id_hash_fd{-1};
    std::string pin_path_prog_array{};
    std::string pin_path_id_to_index{};
  };
  struct Region {
    int start_index{0};
  };

  bool ensureMaps(daemon::HookType hook_type);
  static std::string getPinPath(std::string_view map_name);

  std::unordered_map<daemon::HookType, HookMaps> hook_maps_;
  std::unordered_map<daemon::HookType, std::unordered_map<int, Region>> regions_;
  mutable std::mutex mutex_;
  std::atomic<bool> initialized_{false};

  struct ProgRegistry {
    std::unordered_map<std::string, ProgInfo> map;
    mutable std::mutex mtx;
  };
  ProgRegistry prog_registry_;
};

} // namespace ebpf
} // namespace hebpf
