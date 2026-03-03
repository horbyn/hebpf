// clang-format off
#include "example_hello.h"
#include "src/common/assert.h"
#include "src/common/exception.h"
// clang-format on

namespace hebpf {
namespace services {
namespace examples {

ExampleHelloEbpf::ExampleHelloEbpf(std::unique_ptr<example_hello_bpf> skel)
    : ebpf::EbpfSkelIf<example_hello_bpf>{std::move(skel)} {}

/**
 * @brief 获取 hello 程序名称
 *
 * @return std::string 名称
 */
std::string ExampleHelloEbpf::getName() const { return std::string{SERVICE_NAME_HELLO}; }

/**
 * @brief 启动 hello
 *
 * @param io_ctx io 模块
 * @return true 成功
 * @return false 失败
 */
bool ExampleHelloEbpf::start(std::weak_ptr<io::IoIf> io_ctx) {
  EbpfSkelIf<example_hello_bpf>::start(io_ctx);

  // 打开 BPF skeleton
  this->open();
  if (skel_ == nullptr) {
    return false;
  }

  // 加载并验证 eBPF 程序
  this->load();

  // 附加到挂载点
  this->attach();

  LOG(info, "eBPF programs running! Press Ctrl+C to stop");
  LOG(info, "Run `sudo cat /sys/kernel/debug/tracing/trace_pipe` to view logs");

  return true;
}

/**
 * @brief 停止 hello
 *
 */
void ExampleHelloEbpf::stop() {
  this->detach();
  this->destroy();
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
  return std::make_unique<hebpf::services::examples::ExampleHelloEbpf>();
}
