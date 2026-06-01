#pragma once

// clang-format off
#include "src/services/acl/acl.skel.h"
#include "acl_base.h"
// clang-format on

namespace hebpf {
namespace services {
namespace acl {

class Acl : public AclBase<struct acl_bpf> {
public:
  explicit Acl(std::unique_ptr<acl_bpf> skel = {});

  std::string getName() const override;
};

} // namespace acl
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
