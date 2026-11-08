#pragma once

// clang-format off
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"
#include "pinned_map_if.h"
#include "hebpf_version.h"
// clang-format on

namespace hebpf {
namespace ebpf {

constexpr std::string_view KCONFIG_MAP{HEBPF_PROJECT "_config"};

/**
 * @brief 内核态通用配置
 */
class ConfigMap : public PinnedMapBase {
public:
  explicit ConfigMap(std::string_view map_name, size_t max_entries);

  ConfigMap(const ConfigMap &) = delete;
  ConfigMap &operator=(const ConfigMap &) = delete;

  bool init() override;
  nlohmann::json getDebugStatus() override;

  bool setConfig(__u32 key, __u64 value);
  bool getConfig(__u32 key, __u64 &value) const;

private:
  bpf_map_type getMapType() const override;
  size_t getKeySize() const override;
  size_t getValueSize() const override;

  std::string map_name_{};
  size_t map_size_{0};
};

} // namespace ebpf
} // namespace hebpf
