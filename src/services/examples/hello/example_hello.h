#pragma once

// clang-format off
#include "src/ebpf/ebpf_if.h"
#include "src/services/examples/hello/example_hello.skel.h"
// clang-format on

namespace hebpf {
namespace services {
namespace examples {

class ExampleHelloEbpf : public ebpf::EbpfSkelIf<struct example_hello_bpf> {
public:
  explicit ExampleHelloEbpf(std::unique_ptr<struct example_hello_bpf> skel = {});

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
