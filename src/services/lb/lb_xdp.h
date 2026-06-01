#pragma once

// clang-format off
#include "src/services/lb/lb_xdp.skel.h"
#include "lb_base.h"
// clang-format on

namespace hebpf {
namespace services {
namespace lb {

class LbXdp : public LbBase<struct lb_xdp_bpf> {
public:
  explicit LbXdp(std::unique_ptr<lb_xdp_bpf> skel = {});

  std::string getName() const override;
};

} // namespace lb
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
