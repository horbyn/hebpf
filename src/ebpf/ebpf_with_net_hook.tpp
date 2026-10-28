#pragma once

// clang-format off
#include "ebpf_with_net_hook.h"
#include "src/common/assert.h"
#include "src/common/enum_name.hpp"
#include "src/ebpf/tc_hook.h"
#include "src/ebpf/xdp_hook.h"
#include "src/fd/fd.h"
// clang-format on

namespace hebpf {
namespace ebpf {

template <typename T>
EbpfWithNetHook<T>::EbpfWithNetHook(std::unique_ptr<T> skel)
    : EbpfSkelIf<T>(std::move(skel)), ifindex_{FD_INVALID}, attached_{false} {}

/**
 * @brief 解绑网络 hook
 *
 */
template <typename T>
void EbpfWithNetHook<T>::detach() {
  if (!attached_.load()) {
    return;
  }

  if (hook_ != nullptr) {
    hook_->detach();
    attached_.store(false);
    LOG(debug, "Service {} detached", this->getName());
  }
}

/**
 * @brief 停止程序
 *
 */
template <typename T>
void EbpfWithNetHook<T>::stop() {
  ebpf::EbpfWithNetHook<T>::detach();
  ebpf::EbpfSkelIf<T>::stop();
}

/**
 * @brief 根据 hook 类型和 NIC 索引创建对应的 hook 并附加程序
 *
 * @param hook_type 绑定类型
 * @param id 网络接口索引
 * @return true 成功，false 失败
 */
template <typename T>
bool EbpfWithNetHook<T>::attachGeneric(daemon::HookType hook_type, int id) {
  if (id < 0) {
    LOG(error, "Invalid ifindex: {}", id);
    return false;
  }
  if (attached_.load()) {
    // TODO: 如果已附加且 ifindex/hook 未变，则跳过；否则需要先 detach
    return true;
  }

  auto name = this->getName();
  auto fd = this->getRunningFd(hook_type);
  if (fd < 0) {
    LOG(error, "Cannot get eBPF program fd for {}", name);
    return false;
  }

  switch (hook_type) {
  case daemon::HookType::TC:
    hook_ = std::make_unique<TcHook>(id, TcHook::Direct::INGRESS);
    break;
  case daemon::HookType::XDP_Generic:
    hook_ = std::make_unique<XdpHook>(id, XdpHook::Mode::XDP_GENERIC);
    break;
  default:
    LOG(error, "Unsupported hook type");
    return false;
  }

  if (hook_ == nullptr) {
    LOG(error, "Failed to create {} hook: {}", enumName(hook_type), name);
    return false;
  }

  if (!hook_->attach(std::make_unique<Fd>(fd))) {
    LOG(error, "Failed to attach {} hook: {}", enumName(hook_type), name);
    hook_.reset();
    return false;
  }

  ifindex_ = id;
  attached_.store(true);
  LOG(debug, "Service {} attached on ifindex {}", name, ifindex_);
  return true;
}

} // namespace ebpf
} // namespace hebpf
