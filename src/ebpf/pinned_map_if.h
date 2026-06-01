#pragma once

// clang-format off
#include <string>
#include "nlohmann/json.hpp"
#include "src/log/logger.h"
// clang-format on

namespace hebpf {
namespace ebpf {

/**
 * @brief pinned eBPF map 管理接口
 */
class PinnedMapIf : public log::Loggable<log::Id::ebpf> {
public:
  virtual ~PinnedMapIf() = default;

  virtual bool init() = 0;
  virtual void cleanup() = 0;
  virtual nlohmann::json getDebugStatus() = 0;

  void ensureBpffsMounted();
  void releaseBpffs();
};

} // namespace ebpf
} // namespace hebpf
