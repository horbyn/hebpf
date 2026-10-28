// clang-format off
#include "pinned_ringbuffer_map.h"
#include <bpf/bpf.h>
#include <unistd.h>
#include "src/common/assert.h"
#include "src/common/exception.h"
#include "src/fd/fd.h"
// clang-format on

namespace hebpf {
namespace ebpf {

RingBufferMap::RingBufferMap(std::string_view map_name, size_t map_size) {
  setMapName(map_name);
  setMapSize(map_size);
}

/**
 * @brief 初始化公共 map，若已存在则打开，否则创建并 pin
 * @return true 成功，false 失败
 */
bool RingBufferMap::init() {
  if (initialized_.load()) {
    return true;
  }

  ASSERT(!map_name_.empty());
  ASSERT(map_size_ > 0);

  if (!createPinnedInfo(map_name_, map_size_)) {
    return false;
  }

  initialized_.store(true);
  return true;
}

/**
 * @brief 获取 eBPF ring buffer 的数据信息对接 Debug 服务器
 *
 * @return nlohmann::json JSON 对象
 */
nlohmann::json RingBufferMap::getDebugStatus() { throw EXCEPT("not implemented"); }

/**
 * @brief 设置 pinned map 大小
 *
 * @param size 大小
 */
void RingBufferMap::setMapSize(size_t size) { map_size_ = size; }

/**
 * @brief 设置 pinned map 的名字
 *
 * @param name pinned map 的名称
 */
void RingBufferMap::setMapName(std::string_view name) { map_name_ = std::string{name}; }

/**
 * @brief 获取一个 pinned 的 eBPF ring buffer
 *
 * @param io_ctx IO 对象
 * @param callback 回调函数
 * @return std::unique_ptr<Ringbuffer> ring buffer 对象
 */
std::unique_ptr<Ringbuffer> RingBufferMap::getRingbuffer(std::weak_ptr<io::IoIf> io_ctx,
                                                         Ringbuffer::RingbufferCb callback) {
  if (initialized_.load() == false) {
    if (!init()) {
      throw EXCEPT("Object initialization failed");
    }
  }

  auto rb = std::make_unique<Ringbuffer>(std::make_unique<Fd>(getPinnedFd()), io_ctx,
                                         std::move(callback));
  rb->init();
  return rb;
}

/**
 * @brief 获取 map 类型
 *
 * @return bpf_map_type 类型
 */
bpf_map_type RingBufferMap::getMapType() const { return BPF_MAP_TYPE_RINGBUF; }

/**
 * @brief 获取 key 大小
 *
 * @return size_t 大小
 */
size_t RingBufferMap::getKeySize() const { return 0; }

/**
 * @brief 获取 value 大小
 *
 * @return size_t 大小
 */
size_t RingBufferMap::getValueSize() const { return 0; }

} // namespace ebpf
} // namespace hebpf
