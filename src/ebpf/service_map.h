#pragma once

// clang-format off
#include <dlfcn.h>
#include <unordered_map>
#include "src/daemon/hooks.h"
#include "ebpf_if.h"
// clang-format on

namespace hebpf {
namespace ebpf {

/**
 * @brief eBPF 程序删除器
 *
 */
struct DlCloser {
  void operator()(void *h) const noexcept;
};

struct ServiceKey {
  daemon::HookKey hook;
  std::string name; // 配置文件给出的名称（和 ebpf_id.map 第一列保持一致）
  bool operator==(const ServiceKey &other) const;
};

/**
 * @brief eBPF 程序实例
 *
 */
struct ServiceHandle {
  std::unique_ptr<void, DlCloser> handle; // 动态库 handle
  std::unique_ptr<EbpfIf> instance;       // eBPF 对象
  daemon::ConfigEbpfPtr config;           // eBPF 配置对象
};

struct ServiceKeyHash {
  size_t operator()(const ServiceKey &key) const noexcept;
};

using ServiceMap = std::unordered_map<ServiceKey, ServiceHandle, ServiceKeyHash>;
void to_json(nlohmann::json &json, const ServiceMap &map);

} // namespace ebpf
} // namespace hebpf
