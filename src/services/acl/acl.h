#pragma once

// clang-format off
#include <atomic>
#include <memory>
#include "src/ebpf/ebpf_if.h"
#include "src/ebpf/ebpf_net_hook_if.h"
#include "src/log/logger.h"
#include "src/services/acl/acl.skel.h"
#include "acl_rules.h"
// clang-format on

namespace hebpf {
namespace services {
namespace acl {
constexpr size_t MAX_RULES_SIZE{10240};

class Acl : public ebpf::EbpfSkelIf<struct acl_bpf>, public log::Loggable<log::Id::ebpf> {
public:
  explicit Acl(std::unique_ptr<acl_bpf> skel = {});

  std::string getName() const override;
  void detach() override;
  bool start(std::weak_ptr<io::IoIf> io_ctx = {}) override;
  void stop() override;
  void onConfigUpdate(const nlohmann::json &config) override;

private:
  int getMapFd() const;
  void clearKernelRules();
  void attachTc(int ifindex);

  std::unique_ptr<ebpf::EbpfNetHookIf> hook_;
  int ifindex_;
  AclRules rules_;
  std::atomic<bool> attached_;
};

} // namespace acl
} // namespace services
} // namespace hebpf

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
