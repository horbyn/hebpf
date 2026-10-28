// clang-format off
#include "pinned_map_if.h"
#include <bpf/bpf.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <atomic>
#include <mutex>
#include "src/common/assert.h"
#include "src/common/exception.h"
#include "src/log/logger.h"
#include "ebpf_common.h"
// clang-format on

#ifndef BPF_FS_MAGIC
#define BPF_FS_MAGIC 0xCAFE4A11
#endif

namespace hebpf {
namespace ebpf {

static std::atomic<int> g_mount_refcnt{0};
static std::once_flag g_mount_init_flag{};
static std::atomic<bool> g_mounted_by_us{false};

PinnedMapIf::PinnedMapIf() { ensureBpffsMounted(); }

PinnedMapIf::~PinnedMapIf() { releaseBpffs(); }

/**
 * @brief 确保 BPF 文件系统已挂载（引用计数）
 */
void PinnedMapIf::ensureBpffsMounted() {
  std::call_once(g_mount_init_flag, []() {
    const char *bpffs_path = PATH_BPFFS;
    struct stat st{};
    if (stat(bpffs_path, &st) != 0) {
      if (mkdir(bpffs_path, 0755) != 0 && errno != EEXIST) {
        throw EXCEPT(fmt::format("Failed to create {}: {}", bpffs_path, strerror(errno)));
      }
    }

    struct statfs sfs{};
    if (statfs(bpffs_path, &sfs) == 0 && sfs.f_type == BPF_FS_MAGIC) {
      GLOBAL_LOG(debug, "bpffs already mounted at {}", bpffs_path);
      g_mounted_by_us.store(false);
    } else {
      if (mount("none", bpffs_path, "bpf", 0, nullptr) != 0) {
        throw EXCEPT(fmt::format("Failed to mount bpffs: {}", strerror(errno)));
      }
      GLOBAL_LOG(info, "Mounted bpffs at {}", bpffs_path);
      g_mounted_by_us.store(true);
    }
  });
  ++g_mount_refcnt;
}

/**
 * @brief 释放 BPF 文件系统引用，必要时卸载
 */
void PinnedMapIf::releaseBpffs() {
  if (--g_mount_refcnt == 0 && g_mounted_by_us.load()) {
    if (umount(PATH_BPFFS) != 0) {
      LOG(warn, "Failed to unmount bpffs: {}", strerror(errno));
    } else {
      LOG(info, "Unmounted bpffs at {}", PATH_BPFFS);
      g_mounted_by_us.store(false);
    }
  }
}

PinnedMapBase::~PinnedMapBase() { cleanup(); }

/**
 * @brief 获取 pinned map 的 fd
 *
 * @return int 文件描述符，出错返回 FD_INVALID
 */
int PinnedMapBase::getPinnedFd() const noexcept { return fd_; }

/**
 * @brief 获取 pinned map 的固定位置
 *
 * @return std::string 路径
 */
std::string PinnedMapBase::getPinnedPath() const { return path_; }

/**
 * @brief 创建指定 pinned map
 *
 * @param name 名称
 * @param map_size 表大小
 * @return true 成功
 * @return false 失败
 */
bool PinnedMapBase::createPinnedInfo(std::string_view name, size_t map_size) {
  if (initialized_.load()) {
    return true;
  }

  ASSERT(!name.empty());
  auto map_name = std::string{name};
  auto pin_path = std::string{PATH_BPFFS} + map_name;

  int fd = bpf_obj_get(pin_path.c_str());
  if (fd < 0) {
    fd = bpf_map_create(getMapType(), map_name.c_str(), getKeySize(), getValueSize(), map_size,
                        nullptr);
    if (fd < 0) {
      LOG(error, "Failed to create map {}: {}", map_name, strerror(errno));
      return false;
    }
    if (bpf_obj_pin(fd, pin_path.c_str()) < 0) {
      close(fd);
      LOG(error, "Failed to pin {}: {}", pin_path, strerror(errno));
      return false;
    }
  }

  name_ = map_name;
  fd_ = fd;
  path_ = pin_path;

  LOG(info, "Initialized {} pinned map", map_name);
  return true;
}

/**
 * @brief 清理所有 pinned map
 *
 */
void PinnedMapBase::cleanup() {
  if (!initialized_.load()) {
    return;
  }

  if (fd_ >= 0) {
    close(fd_);
  }
  if (path_.empty() == false) {
    if (unlink(path_.c_str()) != 0 && errno != ENOENT) {
      LOG(warn, "Failed to unlink {}: {}", path_, strerror(errno));
    }
  }
  initialized_.store(false);
}

} // namespace ebpf
} // namespace hebpf
