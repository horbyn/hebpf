// clang-format off
#include "pinned_map_if.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <atomic>
#include <mutex>
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
      GLOBAL_LOG(warn, "Failed to unmount bpffs: {}", strerror(errno));
    } else {
      GLOBAL_LOG(info, "Unmounted bpffs at {}", PATH_BPFFS);
      g_mounted_by_us.store(false);
    }
  }
}

} // namespace ebpf
} // namespace hebpf
