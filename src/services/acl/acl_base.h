#pragma once

// clang-format off
#include "src/ebpf/ebpf_with_net_hook.h"
#include "acl_rules.h"
// clang-format on

namespace hebpf {
namespace services {
namespace acl {

constexpr size_t MAX_RULES_SIZE{10240};

template <typename SkelType>
class AclBase : public ebpf::EbpfWithNetHook<SkelType> {
public:
  explicit AclBase(std::unique_ptr<SkelType> skel = {});

  bool start(std::weak_ptr<io::IoIf> io_ctx = {}) override;
  void stop() override;
  int getMainProgFd() const override;
  nlohmann::json getDebugStatus() const override;
  void onConfigUpdate(daemon::HookType hook_type, int ifindex,
                      const nlohmann::json &config) override;

private:
  // 引入依赖基类的日志相关函数，使 LOG 宏可用

  using ebpf::EbpfWithNetHook<SkelType>::getLevel;
  using ebpf::EbpfWithNetHook<SkelType>::getModuleLogger;

protected:
  int getMapFd() const;
  void clearKernelRules();

  AclRules rules_;
};

} // namespace acl
} // namespace services
} // namespace hebpf

#include "acl_base.tpp"

extern "C" {
EXPORT std::unique_ptr<hebpf::ebpf::EbpfIf> create_service();
}
