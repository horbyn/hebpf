// clang-format off
#include "acl.h"
#include <arpa/inet.h>
#include <bpf/bpf.h>
#include "src/common/assert.h"
#include "src/data/metrics.h"
#include "src/data/kernel_data.h"
#include "src/ebpf/tc_hook.h"
#include "src/fd/fd.h"
#include "acl.bpf.h"
// clang-format on

namespace hebpf {
namespace services {
namespace acl {

Acl::Acl(std::unique_ptr<acl_bpf> skel) : ebpf::EbpfSkelIf<acl_bpf>{std::move(skel)} {
  ifindex_ = INVALID_IFINDEX;
  rules_ = AclRules{};
  attached_.store(false);
}

/**
 * @brief 获取 acl 程序名称
 *
 * @return std::string 名称
 */
std::string Acl::getName() const { return std::string{SERVICE_NAME_ACL}; }

/**
 * @brief acl 解绑 hook
 *
 */
void Acl::detach() {
  if (!attached_.load()) {
    return;
  }

  if (hook_ != nullptr) {
    hook_->detach();
    attached_.store(false);
    LOG(debug, "Service {} detached", SERVICE_NAME_ACL);
  }
}

/**
 * @brief 启动 acl
 *
 * @param io_ctx io 模块
 * @return true 成功
 * @return false 失败
 */
bool Acl::start(std::weak_ptr<io::IoIf> io_ctx) {
  if (!EbpfSkelIf<acl_bpf>::start(io_ctx)) {
    throw EXCEPT(fmt::format("Start {} service failed", SERVICE_NAME_ACL));
  }

  this->open();
  if (skel_ == nullptr) {
    return false;
  }
  this->load();
  this->attach();

  return true;
}

/**
 * @brief 停止 acl
 *
 */
void Acl::stop() {
  this->detach();
  EbpfSkelIf<acl_bpf>::detach();

  this->destroy();
}

/**
 * @brief ACL 配置更新时触发
 *
 * @param config ACL 配置
 */
void Acl::onConfigUpdate(const nlohmann::json &config) {

  AclRules acl_rules{};
  try {
    acl_rules = config;
  } catch (const std::exception &exc) {
    throw EXCEPT(fmt::format("Invalid ACL config: {}", exc.what()));
  }

  clearKernelRules();

  attachTc(acl_rules.getIfindex());

  size_t success = 0;
  const auto &rules = acl_rules.getRules();
  auto map_fd = getMapFd();
  for (const auto &rule : rules) {
    struct ipv4_tuple tuple{};

    auto saddr = rule.getSaddr();
    if (inet_pton(AF_INET, saddr.c_str(), &tuple.saddr) != 1) {
      LOG(warn, "Invalid source IP: {}", saddr);
      continue;
    }

    auto daddr = rule.getDaddr();
    if (inet_pton(AF_INET, daddr.c_str(), &tuple.daddr) != 1) {
      LOG(warn, "Invalid destination IP: {}", daddr);
      continue;
    }

    auto sport = rule.getSport();
    tuple.sport = htons(sport);

    auto dport = rule.getDport();
    tuple.dport = htons(dport);

    auto protocol = rule.getProtocol();
    if (protocol == AclRulesElem::Protocol::TCP) {
      tuple.protocol = IPPROTO_TCP;
    } else if (protocol == AclRulesElem::Protocol::UDP) {
      tuple.protocol = IPPROTO_UDP;
    } else {
      LOG(warn, "Invalid protocol: {}", AclRulesElem::ProtocolToString(protocol));
      continue;
    }

    auto action = rule.getAction();
    uint32_t action_u{};
    if (action == AclRulesElem::Action::DROP) {
      action_u = ACL_ACTION_DENY;
    } else if (action == AclRulesElem::Action::ACCEPT) {
      action_u = ACL_ACTION_ALLOW;
    } else {
      LOG(warn, "Invalid action: {}", AclRulesElem::ActionToString(action));
      continue;
    }

    if (bpf_map_update_elem(map_fd, &tuple, &action_u, BPF_ANY) != 0) {
      LOG(warn, "Failed to insert rule: {} {}:{} -> {}:{} {}",
          AclRulesElem::ProtocolToString(protocol), saddr, sport, daddr, dport,
          AclRulesElem::ActionToString(action));
    } else {
      rules_.addRules(rule);
      ++success;
      if (success >= MAX_RULES_SIZE) {
        // TODO: 目前暂时 hard-code 规则数量为 MAX_RULES_SIZE
        break;
      }
    }
  } // end for()

  LOG(info, "Loaded {} ACL rules", success);
}

/**
 * @brief 获取 eBPF map 文件描述符
 *
 * @return int 文件描述符，出错返回 FD_INVALID
 */
int Acl::getMapFd() const {
  ASSERT(skel_ != nullptr);

  auto map_fd = bpf_map__fd(skel_->maps.krules_acl);
  if (map_fd < 0) {
    throw EXCEPT("Failed to get ACL rules in kernel mode");
  }
  return map_fd;
}

/**
 * @brief 清空内核态规则
 *
 */
void Acl::clearKernelRules() {
  auto map_fd = getMapFd();

  std::vector<struct ipv4_tuple> keys{};
  struct ipv4_tuple first_key{};
  if (bpf_map_get_next_key(map_fd, nullptr, &first_key) == 0) {
    do {
      keys.push_back(first_key);
    } while (bpf_map_get_next_key(map_fd, &first_key, &first_key) == 0);
  }

  for (const auto &k : keys) {
    bpf_map_delete_elem(map_fd, &k);
  }

  rules_.clearRules();
  LOG(debug, "Cleared {} ACL rules in kernel mode", keys.size());
}

/**
 * @brief acl 绑定 TC hook
 *
 * @param ifindex 网络接口索引
 * @note TODO: 暂时 hard-code TC ingress
 */
void Acl::attachTc(int ifindex) {
  if (ifindex < 0) {
    throw EXCEPT(fmt::format("Invalid ifindex: {}", ifindex));
  }

  if (attached_.load()) {
    return;
  }

  auto fd = bpf_program__fd(skel_->progs.hebpf_acl_tc_ingress);
  if (fd < 0) {
    throw EXCEPT(fmt::format("Cannot get TC program fd: {}", SERVICE_NAME_ACL), true);
  }
  hook_ = std::make_unique<ebpf::TcHook>(ifindex, ebpf::TcHook::Direct::INGRESS);
  if (hook_ == nullptr) {
    throw EXCEPT(fmt::format("Cannot create TC hook: {}", SERVICE_NAME_ACL));
  }
  if (!hook_->attach(std::make_unique<Fd>(fd))) {
    throw EXCEPT(fmt::format("Cannot attach TC hook: {}", SERVICE_NAME_ACL));
  }
  ifindex_ = ifindex;

  attached_.store(true);
  LOG(debug, "Service {} attached", SERVICE_NAME_ACL);
}

} // namespace acl
} // namespace services
} // namespace hebpf

/**
 * @brief 动态库导出接口：用来返回 eBPF 对象
 *
 * @return std::unique_ptr<hebpf::ebpf::EbpfIf> 对象
 */
extern "C" std::unique_ptr<hebpf::ebpf::EbpfIf> create_service() {
  return std::make_unique<hebpf::services::acl::Acl>();
}
