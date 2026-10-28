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
class RingBufferMap : public PinnedMapBase {
public:
  explicit RingBufferMap(std::string_view map_name, size_t map_size);

  RingBufferMap(const RingBufferMap &) = delete;
  RingBufferMap &operator=(const RingBufferMap &) = delete;

  bool init() override;
  nlohmann::json getDebugStatus() override;

  void setMapSize(size_t size);
  void setMapName(std::string_view name);
  std::unique_ptr<Ringbuffer> getRingbuffer(std::weak_ptr<io::IoIf> io_ctx,
                                            Ringbuffer::RingbufferCb callback);

private:
  bpf_map_type getMapType() const override;
  size_t getKeySize() const override;
  size_t getValueSize() const override;

  std::string map_name_{};
  size_t map_size_{0};
};

} // namespace ebpf
} // namespace hebpf
