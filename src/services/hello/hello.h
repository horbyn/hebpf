#pragma once

// clang-format off
#include <functional>
#include "src/interface/ebpf_if.h"
#include "src/log/logger.h"
#include "src/services/hello/hello.skel.h"
// clang-format on

namespace hebpf {
namespace services {
namespace hello {

constexpr std::string_view SERVICE_NAME_HELLO{"hello"};

class HelloEbpf : public EbpfIf, public log::Loggable<log::Id::ebpf> {
public:
  explicit HelloEbpf(std::unique_ptr<hello_bpf> skel = nullptr);

  HelloEbpf(const HelloEbpf &) = delete;
  HelloEbpf &operator=(const HelloEbpf &) = delete;
  HelloEbpf(HelloEbpf &&other) noexcept;
  HelloEbpf &operator=(HelloEbpf &&other) noexcept;

  std::string getName() const override;
  void open() override;
  void load() override;
  void attach() override;
  void detach() override;
  void destroy() override;

  explicit operator bool() const;

private:
  std::unique_ptr<hello_bpf> skel_;
};

} // namespace hello
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::EbpfIf> create_service();
}
