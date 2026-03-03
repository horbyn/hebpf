#pragma once

// clang-format off
#include "src/ebpf/ebpf_if.h"
#include "src/log/logger.h"
#include "src/services/examples/hello/example_hello.skel.h"
// clang-format on

namespace hebpf {
namespace services {
namespace examples {

constexpr std::string_view SERVICE_NAME_HELLO{"example_hello"};

class ExampleHelloEbpf : public ebpf::EbpfSkelIf<struct example_hello_bpf>,
                         public log::Loggable<log::Id::ebpf> {
public:
  explicit ExampleHelloEbpf(std::unique_ptr<struct example_hello_bpf> skel = nullptr);

  std::string getName() const override;
  bool start(std::weak_ptr<io::IoIf> io_ctx = {}) override;
  void stop() override;
};

} // namespace examples
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
