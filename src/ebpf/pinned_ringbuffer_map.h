#pragma once

// clang-format off
#include <memory>
#include <string>
#include <string_view>
#include "pinned_map_if.h"
#include "src/ebpf/ringbuffer.h"
// clang-format on

namespace hebpf {
namespace ebpf {

/**
 * @brief 管理 pinned ring buffer map
 */
class RingBufferMap : public PinnedMapIf {
public:
  explicit RingBufferMap(std::string_view pin_path);

  RingBufferMap(const RingBufferMap &) = delete;
  RingBufferMap &operator=(const RingBufferMap &) = delete;

  bool init() override;
  void cleanup() override;
  nlohmann::json getDebugStatus() override;

  void setMapSize(size_t size);
  void setMapName(std::string_view name);
  std::unique_ptr<Ringbuffer> getRingbuffer(std::weak_ptr<io::IoIf> io_ctx,
                                            Ringbuffer::RingbufferCb callback);

private:
  std::string pin_path_;
  std::string map_name_{};
  size_t map_size_{0};
  int map_fd_{-1};
  bool initialized_{false};
};

} // namespace ebpf
} // namespace hebpf
