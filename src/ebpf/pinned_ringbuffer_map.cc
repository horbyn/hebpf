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

RingBufferMap::RingBufferMap(std::string_view pin_path) : pin_path_{std::string{pin_path}} {}

/**
 * @brief 初始化公共 map，若已存在则打开，否则创建并 pin
 * @return true 成功，false 失败
 */
bool RingBufferMap::init() {
  ASSERT(!pin_path_.empty());
  ASSERT(!map_name_.empty());
  ASSERT(map_size_ > 0);

  ensureBpffsMounted();

  map_fd_ = bpf_obj_get(pin_path_.c_str());
  if (map_fd_ >= 0) {
    // 已存在 pinned map
  } else if (errno == ENOENT) {
    // 不存在，创建并 pin
    map_fd_ = bpf_map_create(BPF_MAP_TYPE_RINGBUF, map_name_.c_str(), 0, 0, map_size_, nullptr);
    if (map_fd_ < 0) {
      LOG(error, "bpf_map_create failed for {}: {}", map_name_, strerror(errno));
      return false;
    }
    if (bpf_obj_pin(map_fd_, pin_path_.c_str()) < 0) {
      LOG(error, "bpf_obj_pin failed for {}: {}", pin_path_, strerror(errno));
      close(map_fd_);
      map_fd_ = -1;
      return false;
    }
  } else {
    LOG(error, "bpf_obj_get failed for {}: {}", pin_path_, strerror(errno));
    return false;
  }

  ASSERT(map_fd_ > 0);

  initialized_ = true;
  return true;
}

/**
 * @brief 从文件系统中删除 pinned map
 */
void RingBufferMap::cleanup() {
  if (map_fd_ >= 0) {
    close(map_fd_);
    map_fd_ = -1;
  }
  if (pin_path_.empty() == false) {
    if (unlink(pin_path_.c_str()) != 0 && errno != ENOENT) {
      LOG(warn, "Failed to unlink {}: {}", pin_path_, strerror(errno));
    }
  }
  initialized_ = false;
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
  if (initialized_ == false) {
    if (!init()) {
      throw EXCEPT("Object initialization failed");
    }
  }

  auto rb =
      std::make_unique<Ringbuffer>(std::make_unique<Fd>(map_fd_), io_ctx, std::move(callback));
  rb->init();
  return rb;
}

} // namespace ebpf
} // namespace hebpf
