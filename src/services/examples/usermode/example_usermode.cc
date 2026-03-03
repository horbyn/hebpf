// clang-format off
#include "example_usermode.h"
#include "src/common/exception.h"
#include "src/ebpf/ringbuffer.h"
#include "src/fd/fd.h"
// clang-format on

namespace hebpf {
namespace services {
namespace examples {

ExampleUsermodeEbpf::ExampleUsermodeEbpf(std::unique_ptr<example_usermode_bpf> skel)
    : ebpf::EbpfSkelIf<example_usermode_bpf>{std::move(skel)} {}

/**
 * @brief 获取 usermode 程序名称
 *
 * @return std::string 名称
 */
std::string ExampleUsermodeEbpf::getName() const { return std::string{SERVICE_NAME_USERMODE}; }

/**
 * @brief 加载 usermode
 *
 */
void ExampleUsermodeEbpf::load() {
  ebpf::EbpfSkelIf<example_usermode_bpf>::load();

  auto fd = bpf_map__fd(skel_->maps.rb);
  if (fd < 0) {
    throw EXCEPT(fmt::format("Cannot get ringbuffer fd: {}", SERVICE_NAME_USERMODE), true);
  }
  ringbuff_ = std::make_unique<ebpf::Ringbuffer>(
      std::make_unique<Fd>(fd), EbpfSkelIf<example_usermode_bpf>::io_ctx_,
      ebpf::Ringbuffer::RingbufferCb{
          "ExampleUsermodeEbpf::onEvent()",
          [this](void *data, size_t len) { return onEvent(data, len); }});
  ringbuff_->init();
}

/**
 * @brief 启动 usermode
 *
 * @param io_ctx io 模块
 * @return true 成功
 * @return false 失败
 */
bool ExampleUsermodeEbpf::start(std::weak_ptr<io::IoIf> io_ctx) {
  EbpfSkelIf<example_usermode_bpf>::start(io_ctx);

  this->open();
  if (skel_ == nullptr) {
    return false;
  }
  this->load();
  this->attach();

  return true;
}

/**
 * @brief 停止 usermode
 *
 */
void ExampleUsermodeEbpf::stop() {
  this->detach();
  this->destroy();
}

/**
 * @brief 内核事件到达
 *
 * @param data 内核数据
 * @param data_sz 内核数据长度
 * @return int 成功返回 0，失败返回 -1
 */
int ExampleUsermodeEbpf::onEvent(void *data, size_t data_sz) {
  if (data_sz != sizeof(struct execve_event)) {
    GLOBAL_LOG(error, "{} eBPF cb: invalid event size: {}", SERVICE_NAME_USERMODE, data_sz);
    return -1;
  }

  const struct execve_event *event = static_cast<const struct execve_event *>(data);
  if (event != nullptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    GLOBAL_LOG(info, "{}: pid={}, COMM={}", SERVICE_NAME_USERMODE, event->pid, event->comm);
  }
  return 0;
}

} // namespace examples
} // namespace services
} // namespace hebpf

/**
 * @brief 动态库导出接口：用来返回 eBPF 对象
 *
 * @return std::unique_ptr<hebpf::ebpf::EbpfIf> 对象
 */
extern "C" std::unique_ptr<hebpf::ebpf::EbpfIf> create_service() {
  return std::make_unique<hebpf::services::examples::ExampleUsermodeEbpf>();
}
