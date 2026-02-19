// clang-format off
#include "hello.h"
#include "src/common/assert.h"
#include "src/common/except.h"
// clang-format on

namespace hebpf {
namespace services {
namespace hello {

HelloEbpf::HelloEbpf(std::unique_ptr<hello_bpf> skel) : skel_{std::move(skel)} {}

HelloEbpf::HelloEbpf(HelloEbpf &&other) noexcept : skel_{std::move(other.skel_)} {
  other.skel_ = nullptr;
}

HelloEbpf &HelloEbpf::operator=(HelloEbpf &&other) noexcept {
  if (this != &other) {
    skel_ = std::move(other.skel_);
    other.skel_ = nullptr;
  }
  return *this;
}

/**
 * @brief 打开 eBPF 程序
 */
void HelloEbpf::open() {
  if (skel_ == nullptr) {
    skel_ = std::unique_ptr<hello_bpf>(hello_bpf::open(nullptr));
  }
}

/**
 * @brief 加载 eBPF 程序
 *
 * @note 加载失败会抛出异常
 */
void HelloEbpf::load() {
  ASSERT(skel_ != nullptr);
  auto ret = skel_->load(skel_.get());
  if (ret < 0) {
    throw EXCEPT("Failed to load BPF program", true);
  }
}

/**
 * @brief 将 eBPF 程序绑定到 hook
 *
 * @note 绑定失败会抛出异常
 */
void HelloEbpf::attach() {
  ASSERT(skel_ != nullptr);
  auto ret = skel_->attach(skel_.get());
  if (ret < 0) {
    throw EXCEPT("Failed to attach BPF program", true);
  }
}

/**
 * @brief 取消 eBPF 绑定
 *
 */
void HelloEbpf::detach() {
  ASSERT(skel_ != nullptr);
  skel_->detach(skel_.get());
}

/**
 * @brief 销毁 eBPF 程序
 *
 */
void HelloEbpf::destroy() {
  ASSERT(skel_ != nullptr);
  skel_->destroy(skel_.get());
  skel_.release();
}

HelloEbpf::operator bool() const { return skel_ != nullptr; }

} // namespace hello
} // namespace services
} // namespace hebpf
