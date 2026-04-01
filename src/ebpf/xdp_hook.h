#pragma once

// clang-format off
#include <bpf/libbpf.h>
#include "ebpf_net_hook_if.h"
// clang-format on

namespace hebpf {
namespace ebpf {

class XdpHook : public EbpfNetHookIf {
public:
  enum class Mode : uint8_t { XDP_GENERIC, XDP_NATIVE, XDP_OFFLOAD, UNKNOWN };

  explicit XdpHook(int ifindex, Mode mode);

  int do_attach(int prog_fd) override;
  int do_detach() override;

private:
  unsigned int getFlags() const;

  Mode mode_;
  bool hook_created_;
};

} // namespace ebpf
} // namespace hebpf
