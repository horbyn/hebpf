#pragma once

// clang-format off
#include "ebpf_if.h"
#include "src/common/assert.h"
#include "src/common/exception.h"
// clang-format on

namespace hebpf {
namespace ebpf {

template <typename T>
EbpfSkelIf<T>::EbpfSkelIf(std::unique_ptr<T> skel) : skel_{std::move(skel)} {}

template <typename T>
EbpfSkelIf<T>::EbpfSkelIf(EbpfSkelIf &&other) noexcept : skel_{std::move(other.skel_)} {
  other.skel_ = nullptr;
}

template <typename T>
EbpfSkelIf<T> &EbpfSkelIf<T>::operator=(EbpfSkelIf &&other) noexcept {
  if (this != &other) {
    skel_ = std::move(other.skel_);
    other.skel_ = nullptr;
  }
  return *this;
}

template <typename T>
EbpfSkelIf<T>::operator bool() const {
  return skel_ != nullptr;
}

/**
 * @brief 打开 eBPF 程序
 */
template <typename T>
void EbpfSkelIf<T>::open() {
  if (skel_ == nullptr) {
    skel_ = std::unique_ptr<T>(T::open(nullptr));
  }
}

/**
 * @brief 加载 eBPF 程序
 *
 * @note 加载失败会抛出异常
 */
template <typename T>
void EbpfSkelIf<T>::load() {
  ASSERT(skel_ != nullptr);
  if (skel_->load(skel_.get()) < 0) {
    throw EXCEPT("Failed to load BPF program", true);
  }
}

/**
 * @brief 将 eBPF 程序绑定到 hook
 *
 * @note 绑定失败会抛出异常
 */
template <typename T>
void EbpfSkelIf<T>::attach() {
  ASSERT(skel_ != nullptr);
  if (skel_->attach(skel_.get()) < 0) {
    throw EXCEPT("Failed to attach BPF program", true);
  }
}

/**
 * @brief 取消 eBPF 绑定
 *
 */
template <typename T>
void EbpfSkelIf<T>::detach() {
  ASSERT(skel_ != nullptr);
  skel_->detach(skel_.get());
}

/**
 * @brief 销毁 eBPF 程序
 *
 */
template <typename T>
void EbpfSkelIf<T>::destroy() {
  ASSERT(skel_ != nullptr);
  skel_->destroy(skel_.get());
  skel_.release(); // libbpf 的 skeleton 在 destroy()（上一行代码）会释放内存
}

/**
 * @brief 启动 eBPF 程序
 *
 * @param io_ctx io 模块
 * @note io 模块只有存在的时候，才会赋值
 *
 * @return true 启动成功
 * @return false 启动失败
 */
template <typename T>
bool EbpfSkelIf<T>::start(std::weak_ptr<io::IoIf> io_ctx) {
  if (io_ctx.lock() != nullptr) {
    io_ctx_ = io_ctx;
  }
  return static_cast<bool>(io_ctx_.lock());
}

/**
 * @brief 获取 eBPF 内核态程序收集的状态信息
 *
 * @return nlohmann::json JSON 对象
 */
template <typename T>
nlohmann::json EbpfSkelIf<T>::getStatus() const {
  return nlohmann::json();
}

/**
 * @brief 修改 eBPF 程序配置
 *
 * @param config 配置对象
 */
template <typename T>
void EbpfSkelIf<T>::onConfigUpdate(const nlohmann::json &config) {
  (void)config;
  return;
}

} // namespace ebpf
} // namespace hebpf
