#pragma once

// clang-format off
#include "lb_base.h"
#include <cstring>
#include <bpf/bpf.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <boost/asio.hpp>
#include "lb_common.h"
// clang-format on

namespace hebpf {
namespace services {
namespace lb {

template <typename SkelType>
LbBase<SkelType>::LbBase(std::unique_ptr<SkelType> skel)
    : ebpf::EbpfWithNetHook<SkelType>(std::move(skel)), rules_{} {}

/**
 * @brief 启动 LB
 *
 * @param io_ctx io 模块
 * @return true 成功
 * @return false 失败
 */
template <typename SkelType>
bool LbBase<SkelType>::start(std::weak_ptr<io::IoIf> io_ctx) {
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
 * @brief 停止 LB
 *
 */
template <typename SkelType>
void LbBase<SkelType>::stop() {
  // hebpf::ebpf::EbpfWithNetHook<SkelType>::detach();
  hebpf::ebpf::EbpfSkelIf<SkelType>::detach();

  this->destroy();
}

/**
 * @brief 获取 LB 程序 fd
 *
 * @return int fd
 */
template <typename SkelType>
int LbBase<SkelType>::getMainProgFd() const {
  if constexpr (std::is_same_v<SkelType, struct lb_xdp_bpf>) {
    return this->skel_ != nullptr ? bpf_program__fd(this->skel_->progs.hebpf_lb_xdp_gen) : -1;
  } else {
    return this->skel_ != nullptr ? bpf_program__fd(this->skel_->progs.hebpf_lb_tc) : -1;
  }
}

/**
 * @brief 获取 LB 内核态程序收集的状态信息对接 Debug 服务器
 *
 * @return nlohmann::json JSON 对象
 */
template <typename SkelType>
nlohmann::json LbBase<SkelType>::getDebugStatus() const {
  return rules_;
}

/**
 * @brief LB 配置更新时触发
 *
 * @param hook_type hook 类型
 * @param ifindex 网卡索引
 * @param config LB 配置
 */
template <typename SkelType>
void LbBase<SkelType>::onConfigUpdate([[maybe_unused]] daemon::HookType hook_type,
                                      [[maybe_unused]] int ifindex, const nlohmann::json &config) {

  decltype(rules_) lb_rules{};
  try {
    lb_rules = config;
  } catch (const std::exception &exc) {
    throw EXCEPT(fmt::format("Invalid LB config: {}", exc.what()));
  }

  clearKernelRules();

  auto local_mac_fd = getLocalMacFd();
  __u32 key = 0;
  auto local_mac = lb_rules.getLocalMac();
  unsigned char mac[6];
  if (sscanf(local_mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3],
             &mac[4], &mac[5]) != 6) {
    LOG(warn, "Invalid MAC address: {}", local_mac);
  }
  if (bpf_map_update_elem(local_mac_fd, &key, mac, BPF_ANY) != 0) {
    LOG(warn, "Failed to update lb_lomac map");
  }
  rules_.setLocalMac(local_mac);

  size_t success = 0;
  size_t pool_next = 0;
  const auto &rules = lb_rules.getRules();
  auto vip_hash_fd = getVipBackendsFd();
  auto backend_array_fd = getBackendsPoolFd();
  for (const auto &rule : rules) {
    boost::system::error_code ec{};
    auto vip_addr = boost::asio::ip::make_address_v4(rule.getVip(), ec);
    if (ec) {
      LOG(warn, "{} is not a IPv4 format address", rule.getVip());
      continue;
    }
    auto vport = rule.getVport();
    struct vip_key vkey{.vip = htonl(vip_addr.to_uint()), .vport = htons(vport), .padding = 0};

    const auto &rules_be = rule.getBackends();
    size_t be_count = std::min(rules_be.size(), static_cast<size_t>(MAX_BACKENDS_PER_VIP));
    struct vip_group groups{};
    groups.count = be_count;
    groups.padding = 0;

    for (size_t i = 0; i < be_count; ++i) {
      if (pool_next >= TOTAL_BACKENDS) {
        LOG(warn, "Backend pool exhausted, miss the rest of backends");
        break;
      }
      auto idx = pool_next++;

      auto rules_be_elem = rules_be[i];
      struct backend_info binfo{};

      auto mac_str = rules_be_elem.getMac();
      if (sscanf(mac_str.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &binfo.mac[0], &binfo.mac[1],
                 &binfo.mac[2], &binfo.mac[3], &binfo.mac[4], &binfo.mac[5]) != 6) {
        LOG(warn, "Invalid MAC address: {}", mac_str);
        continue;
      }

      binfo.padding = 0;
      if (bpf_map_update_elem(backend_array_fd, &idx, &binfo, BPF_ANY) != 0) {
        LOG(warn, "Failed to insert backend {} into pool, which MAC address is {}", idx, mac_str);
        continue;
      }

      groups.backends[i] = idx;
    } // end for()

    if (bpf_map_update_elem(vip_hash_fd, &vkey, &groups, BPF_ANY) != 0) {
      LOG(warn, "Failed to update group for VIP:VPORT ({}:{})", vip_addr.to_string(), vport);
    } else {
      rules_.addRules(rule);
      ++success;
      if (success >= MAX_RULES_SIZE) {
        // TODO: 目前暂时 hard-code 规则数量为 MAX_RULES_SIZE
        break;
      }
    }
  } // end for()
}

/**
 * @brief 获取 VIP 后端哈希表文件描述符
 *
 * @return int 文件描述符，出错抛异常
 */
template <typename SkelType>
int LbBase<SkelType>::getVipBackendsFd() const {
  ASSERT(this->skel_ != nullptr);

  auto map_fd = bpf_map__fd(this->skel_->maps.vip_backends);
  if (map_fd < 0) {
    throw EXCEPT("Failed to get LB hash rules in kernel mode");
  }
  return map_fd;
}

/**
 * @brief 获取后端数组文件描述符
 *
 * @return int 文件描述符，出错抛异常
 */
template <typename SkelType>
int LbBase<SkelType>::getBackendsPoolFd() const {
  ASSERT(this->skel_ != nullptr);

  auto map_fd = bpf_map__fd(this->skel_->maps.backend_pool);
  if (map_fd < 0) {
    throw EXCEPT("Failed to get LB array rules in kernel mode");
  }
  return map_fd;
}

/**
 * @brief 获取本地 MAC map 文件描述符
 *
 * @return int 文件描述符，出错抛异常
 */
template <typename SkelType>
int lb::LbBase<SkelType>::getLocalMacFd() const {
  ASSERT(this->skel_ != nullptr);

  auto map_fd = bpf_map__fd(this->skel_->maps.lb_lomac);
  if (map_fd < 0) {
    throw EXCEPT("Failed to get LB array rules in kernel mode");
  }
  return map_fd;
}

/**
 * @brief 清空内核态规则
 *
 */
template <typename SkelType>
void LbBase<SkelType>::clearKernelRules() {

  // 清空 VIP 后端哈希表
  auto vip_hash_fd = getVipBackendsFd();
  std::vector<struct vip_group> groups{};
  struct vip_group first_group{};
  if (bpf_map_get_next_key(vip_hash_fd, nullptr, &first_group) == 0) {
    do {
      groups.push_back(first_group);
    } while (bpf_map_get_next_key(vip_hash_fd, &first_group, &first_group) == 0);
  }

  for (const auto &elem : groups) {
    bpf_map_delete_elem(vip_hash_fd, &elem);
  }

  // 清空后端数组
  auto backend_array_fd = getBackendsPoolFd();
  std::vector<struct backend_info> backends{};
  struct backend_info first_backend{};
  if (bpf_map_get_next_key(backend_array_fd, nullptr, &first_backend) == 0) {
    do {
      backends.push_back(first_backend);
    } while (bpf_map_get_next_key(backend_array_fd, &first_backend, &first_backend) == 0);
  }

  for (const auto &elem : backends) {
    bpf_map_delete_elem(backend_array_fd, &elem);
  }

  rules_.clearRules();
  LOG(debug, "Cleared {} LB rules in kernel mode", groups.size());
}

} // namespace lb
} // namespace services
} // namespace hebpf
