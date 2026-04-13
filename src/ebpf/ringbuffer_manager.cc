// clang-format off
#include "ringbuffer_manager.h"
#include <mntent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <bpf/bpf.h>
#include "src/common/assert.h"
#include "src/common/exception.h"
#include "src/fd/fd.h"
// clang-format on

namespace hebpf {
namespace ebpf {

/**
 * @brief BPF 文件系统挂载
 *
 */
static void ensureBpffsMounted() {
  const char *bpffs_path = PATH_BPFFS;
  struct stat st_fs{};
  if (stat(bpffs_path, &st_fs) == 0) {
    FILE *fp = setmntent("/proc/mounts", "r");
    if (fp != nullptr) {
      struct mntent *mnt{};
      while ((mnt = getmntent(fp)) != nullptr) {
        if (strcmp(mnt->mnt_dir, bpffs_path) == 0 && strcmp(mnt->mnt_type, "bpf") == 0) {
          endmntent(fp);
          GLOBAL_LOG(info, "Already mount bpffs");
          return;
        }
      }
      endmntent(fp);
    }
  }

  // 未挂载，尝试挂载
  if (mount("none", bpffs_path, "bpf", 0, nullptr) != 0) {
    // 如果目录不存在，先创建
    if (errno == ENOENT) {
      mkdir(bpffs_path, 0755);
      if (mount("none", bpffs_path, "bpf", 0, nullptr) != 0) {
        throw EXCEPT(
            fmt::format("Failed to mount bpffs: {}", std::system_category().message(errno)));
      }
    } else {
      throw EXCEPT(fmt::format("Failed to mount bpffs: {}", std::system_category().message(errno)));
    }
  }
  GLOBAL_LOG(info, "Mount bpffs successed");
}

RingbufferManager::RingbufferManager(std::string_view pin_path)
    : pin_path_{std::string{pin_path}} {}

/**
 * @brief 获取一个 pinned 的 eBPF ring buffer
 *
 * @param io_ctx IO 对象
 * @param callback 回调函数
 * @param map_size pinned map 的大小
 * @param map_name pinned map 的名称
 * @return std::unique_ptr<Ringbuffer> ring buffer 对象
 */
std::unique_ptr<Ringbuffer>
RingbufferManager::getPinningRingbuffer(std::weak_ptr<io::IoIf> io_ctx,
                                        Ringbuffer::RingbufferCb callback, size_t map_size,
                                        std::string_view map_name) {
  ensureBpffsMounted();

  int map_fd = bpf_obj_get(pin_path_.c_str());
  if (map_fd >= 0) {
    // 已存在 pinned map
  } else if (errno == ENOENT) {

    // 不存在 pinned map，先创建然后 pin
    map_fd = bpf_map_create(BPF_MAP_TYPE_RINGBUF, std::string{map_name}.c_str(), 0, 0, map_size,
                            nullptr);
    if (map_fd < 0) {
      throw EXCEPT(fmt::format("bpf_map_create failed: {}", std::system_category().message(errno)));
    }
    if (bpf_obj_pin(map_fd, pin_path_.c_str()) < 0) {
      close(map_fd);
      throw EXCEPT(fmt::format("bpf_obj_pin failed: {}", std::system_category().message(errno)));
    }
  } else {
    throw EXCEPT(fmt::format("bpf_obj_get failed: {}", std::system_category().message(errno)));
  }

  ASSERT(map_fd > 0);

  auto rb = std::make_unique<Ringbuffer>(std::make_unique<Fd>(map_fd), io_ctx, std::move(callback));
  rb->init();

  return rb;
}

/**
 * @brief 从文件系统中删除 pinned map
 *
 */
void RingbufferManager::unpin() {
  if (!pin_path_.empty()) {
    if (unlink(pin_path_.c_str()) != 0 && errno != ENOENT) {
      LOG(warn, "Failed to unlink pinned {}: {}", pin_path_, std::system_category().message(errno));
    } else {
      LOG(debug, "Unlinked pinned {}", pin_path_);
    }
  }
}

} // namespace ebpf
} // namespace hebpf
