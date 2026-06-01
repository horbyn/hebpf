// clang-format off
#include "acl_xdp.h"
// clang-format on

namespace hebpf {
namespace services {
namespace acl {

AclXdp::AclXdp(std::unique_ptr<acl_xdp_bpf> skel) : AclBase<acl_xdp_bpf>{std::move(skel)} {}

/**
 * @brief 获取 acl 程序名称
 *
 * @return std::string 名称
 */
std::string AclXdp::getName() const { return std::string{SERVICE_NAME_ACL}; }

} // namespace acl
} // namespace services
} // namespace hebpf

/**
 * @brief 动态库导出接口：用来返回 eBPF 对象
 *
 * @return std::unique_ptr<hebpf::ebpf::EbpfIf> 对象
 */
extern "C" std::unique_ptr<hebpf::ebpf::EbpfIf> create_service() {
  return std::make_unique<hebpf::services::acl::AclXdp>();
}
