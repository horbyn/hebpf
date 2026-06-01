#pragma once

// clang-format off
#include "acl_base.h"
#include <arpa/inet.h>
#include <bpf/bpf.h>
#include "src/common/assert.h"
#include "src/data/metrics.h"
#include "src/data/kernel_data.h"
#include "acl.bpf.h"
// clang-format on

namespace hebpf {
namespace services {
namespace acl {

template <typename SkelType>
AclBase<SkelType>::AclBase(std::unique_ptr<SkelType> skel)
    : ebpf::EbpfWithNetHook<SkelType>(std::move(skel)), rules_{} {}

/**
 * @brief 启动 acl
 *
 * @param io_ctx io 模块
 * @return true 成功
 * @return false 失败
 */
template <typename SkelType>
bool AclBase<SkelType>::start(std::weak_ptr<io::IoIf> io_ctx) {
  if (!ebpf::EbpfSkelIf<SkelType>::start(io_ctx)) {
    throw EXCEPT(fmt::format("Start {} service failed", this->getName()));
  }

  this->open();
  if (this->skel_ == nullptr) {
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
template <typename SkelType>
void AclBase<SkelType>::stop() {
  // ebpf::EbpfWithNetHook<SkelType>::detach();
  ebpf::EbpfSkelIf<SkelType>::detach();

  this->destroy();
}

/**
 * @brief 获取 ACL 程序 fd
 *
 * @return int fd
 */
template <typename SkelType>
int AclBase<SkelType>::getMainProgFd() const {
  if constexpr (std::is_same_v<SkelType, struct acl_bpf>) {
    return this->skel_ ? bpf_program__fd(this->skel_->progs.hebpf_acl_tc_ingress) : -1;
  } else { // acl_xdp_bpf
    return this->skel_ ? bpf_program__fd(this->skel_->progs.hebpf_acl_xdp_generic) : -1;
  }
}

/**
 * @brief 获取 ACL 内核态程序收集的状态信息对接 Debug 服务器
 *
 * @return nlohmann::json JSON 对象
 */
template <typename SkelType>
nlohmann::json AclBase<SkelType>::getDebugStatus() const {
  return rules_;
}

/**
 * @brief ACL 配置更新时触发
 *
 * @param hook_type hook 类型
 * @param ifindex 网卡索引
 * @param config ACL 配置
 */
template <typename SkelType>
void AclBase<SkelType>::onConfigUpdate([[maybe_unused]] daemon::HookType hook_type,
                                       [[maybe_unused]] int ifindex, const nlohmann::json &config) {
  AclRules acl_rules{};
  try {
    acl_rules = config;
  } catch (const std::exception &exc) {
    throw EXCEPT(fmt::format("Invalid ACL config: {}", exc.what()));
  }

  clearKernelRules();

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
template <typename SkelType>
int AclBase<SkelType>::getMapFd() const {
  ASSERT(this->skel_ != nullptr);

  auto map_fd = bpf_map__fd(this->skel_->maps.hebpf_krules_acl);
  if (map_fd < 0) {
    throw EXCEPT("Failed to get ACL rules in kernel mode");
  }
  return map_fd;
}

/**
 * @brief 清空内核态规则
 *
 */
template <typename SkelType>
void AclBase<SkelType>::clearKernelRules() {
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

} // namespace acl
} // namespace services
} // namespace hebpf
