// clang-format off
#include "ebpf_net_hook_if.h"
#include "src/common/assert.h"
// clang-format on

namespace hebpf {
namespace ebpf {

EbpfNetHookIf::EbpfNetHookIf(int ifindex) : ifindex_{ifindex}, attached_{false}, prog_fd_{} {}

EbpfNetHookIf::~EbpfNetHookIf() {
  if (attached_) {
    detach();
  }
}

/**
 * @brief 获取网卡索引
 *
 * @return int 索引
 */
int EbpfNetHookIf::getIfindex() const noexcept { return ifindex_; }

/**
 * @brief 判断是否已经绑定网络 hook
 *
 * @return true 是的
 * @return false 不是
 */
bool EbpfNetHookIf::isAttached() const noexcept { return attached_; }

/**
 * @brief 绑定网络 hook
 *
 * @param fd eBPF 程序文件描述符对象
 * @return true 绑定成功
 * @return false 绑定失败
 */
bool EbpfNetHookIf::attach(std::unique_ptr<FdIf> fd) {
  ASSERT(fd != nullptr);

  if (attached_) {
    LOG(warn, "Already attach");
    return false;
  }

  int err = pre_attach_check();
  if (err < 0) {
    LOG(error, "Pre-attach check failed");
    return false;
  }

  err = do_attach(fd->fd());
  if (err == 0) {
    attached_ = true;
    prog_fd_ = std::move(fd);
  } else {
    LOG(error, "Attach failed");
  }

  return err == 0 ? true : false;
}

/**
 * @brief 取消绑定网络 hook
 *
 * @return true 取消成功
 * @return false 取消失败
 */
bool EbpfNetHookIf::detach() {
  if (!attached_) {
    LOG(warn, "Already detach");
    return true;
  }

  int err = do_detach();
  if (err == 0) {
    attached_ = false;
    prog_fd_.reset();
  } else {
    LOG(error, "Detach failed");
  }

  post_detach_cleanup();

  return err == 0 ? true : false;
}

} // namespace ebpf
} // namespace hebpf
