#pragma once

// clang-format off
#include <bpf/bpf.h>
#include <atomic>
#include <string>
#include "nlohmann/json.hpp"
#include "src/daemon/configs.hpp"
#include "src/fd/fd_if.h"
#include "src/log/logger.h"
#include "ebpf_common.h"
#include "hebpf_version.h"
// clang-format on

namespace hebpf {
namespace ebpf {

/**
 * @brief pinned eBPF map 管理接口
 * @note 负责 pinned 目录的创建与释放
 */
class PinnedMapIf : public log::Loggable<log::Id::ebpf> {
public:
  explicit PinnedMapIf();
  virtual ~PinnedMapIf();

  virtual bool init() = 0;
  virtual nlohmann::json getDebugStatus() = 0;

  void ensureBpffsMounted();
  void releaseBpffs();
};

constexpr std::string_view MAP_NAME_PROG_ARRAY{HEBPF_PROJECT "_parray"};
constexpr std::string_view MAP_NAME_ID_TO_INDEX{HEBPF_PROJECT "_idhash"};

/**
 * @brief 负责 pinned map 的创建与释放
 *
 */
class PinnedMapBase : public PinnedMapIf {
public:
  virtual ~PinnedMapBase();

  int getPinnedFd() const noexcept;
  std::string getPinnedPath() const;

  bool createPinnedInfo(std::string_view name, size_t map_size);
  void cleanup();

protected:
  virtual bpf_map_type getMapType() const = 0;
  virtual size_t getKeySize() const = 0;
  virtual size_t getValueSize() const = 0;

  std::atomic<bool> initialized_{false};

private:
  std::string name_;   // pinned map 名称
  int fd_{FD_INVALID}; // pinned map fd
  std::string path_;   // pinned map 路径
};

} // namespace ebpf
} // namespace hebpf
