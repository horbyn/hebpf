#pragma once

// clang-format off
#include "src/services/acl/acl_xdp.skel.h"
#include "acl_base.h"
// clang-format on

namespace hebpf {
namespace services {
namespace acl {

class AclXdp : public AclBase<struct acl_xdp_bpf> {
public:
  explicit AclXdp(std::unique_ptr<acl_xdp_bpf> skel = {});

  std::string getName() const override;
};

} // namespace acl
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
