// clang-format off
#include "lb_xdp.h"
// clang-format on

namespace hebpf {
namespace services {
namespace lb {

LbXdp::LbXdp(std::unique_ptr<lb_xdp_bpf> skel) : LbBase(std::move(skel)) {}

/**
 * @brief 获取 LB 程序名称
 *
 * @return std::string 名称
 */
std::string LbXdp::getName() const { return std::string{SERVICE_NAME_LB}; }

} // namespace lb
} // namespace services
} // namespace hebpf

/**
 * @brief 动态库导出接口：用来返回 eBPF 对象
 *
 * @return std::unique_ptr<hebpf::ebpf::EbpfIf> 对象
 */
extern "C" std::unique_ptr<hebpf::ebpf::EbpfIf> create_service() {
  return std::make_unique<hebpf::services::lb::LbXdp>();
}
