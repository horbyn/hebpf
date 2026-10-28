// clang-format off
#include "pinned_prog_array.h"
#include "src/common/assert.h"
#include "pinned_map_data.h"
// clang-format on

namespace hebpf {
namespace ebpf {

/**
 * @brief 初始化公共 map，若已存在则打开，否则创建并 pin
 * @note 会创建多个 pinned map，只要有一个失败就全部失败
 *
 * @return true 成功，false 失败
 */
bool PinnedProgArray::init() {
  if (initialized_.load()) {
    return true;
  }

  if (!createPinnedInfo(MAP_NAME_PROG_ARRAY, MAX_CHAIN_PROGS)) {
    return false;
  }

  initialized_.store(true);
  return true;
}

/**
 * @brief 获取 pinned eBPF map 的数据信息对接 Debug 服务器
 *
 * @return nlohmann::json JSON 对象
 */
nlohmann::json PinnedProgArray::getDebugStatus() {
  std::vector<EbpfArray> result{};

  auto array_fd = getPinnedFd();
  ASSERT(array_fd >= 0);

  std::vector<uint32_t> array{};
  for (__u32 key = 0; key < MAX_CHAIN_PROGS; ++key) {
    __u32 index{};
    if (bpf_map_lookup_elem(array_fd, &key, &index) == 0) {
      if (index != 0) {
        array.push_back(index);
      }
    }
  } // end for(array)

  result.emplace_back(getPinnedPath(), array);

  return result;
}

/**
 * @brief 更新 pinned map
 *
 * @param map 程序数组（原地修改）
 * @param hash_obj 哈希表对象
 * @param service_map 服务对象列表
 * @return true 全部成功
 * @return false 部分失败
 */
bool PinnedProgArray::updateMap(daemon::HookMap &map, const PinnedHash *hash_obj,
                                std::shared_ptr<ServiceMap> service_map) {
  if (!initialized_.load()) {
    map.clear(); // 全部删除
    LOG(error, "Object not initialized");
    return false;
  }
  if (service_map == nullptr) {
    map.clear(); // 全部删除
    LOG(error, "No services running");
    return false;
  }

  auto result = true; // 假设全部成功
  for (auto iter = map.begin(); iter != map.end();) {
    auto hash_fd = hash_obj->getPinnedFd();
    const auto &hook_key = iter->first;
    auto &prog_list = iter->second;

    for (auto iter_prog = prog_list.begin(); iter_prog != prog_list.end();) {
      auto prog = *iter_prog;
      auto running_fd = getServiceFd(service_map, hook_key, prog->getName());
      if (!updateMapElem(hook_key, prog, hash_fd, running_fd)) {
        iter_prog = prog_list.erase(iter_prog);
        result = false; // 部分失败
      } else {
        ++iter_prog;
      }
    } // end for(prog list)

    if (prog_list.empty()) {
      iter = map.erase(iter);
    } else {
      ++iter;
    }

  } // end for(map)

  return result;
}

/**
 * @brief 获取 map 类型
 *
 * @return bpf_map_type 类型
 */
bpf_map_type PinnedProgArray::getMapType() const { return BPF_MAP_TYPE_PROG_ARRAY; }

/**
 * @brief 获取 key 大小
 *
 * @return size_t 大小
 */
size_t PinnedProgArray::getKeySize() const { return sizeof(__u32); }

/**
 * @brief 获取 value 大小
 *
 * @return size_t 大小
 */
size_t PinnedProgArray::getValueSize() const { return sizeof(__u32); }

/**
 * @brief 获取 eBPF 程序运行时 fd
 *
 * @param service_map 服务列表
 * @param hook_key 服务的键
 * @param name 服务名称
 * @return int 运行时 fd，出错返回 FD_INVALID
 */
int PinnedProgArray::getServiceFd(std::shared_ptr<ServiceMap> service_map,
                                  const daemon::HookKey &hook_key, std::string_view name) const {
  int service_fd = FD_INVALID;
  ASSERT(service_map != nullptr);

  auto iter = service_map->find(ServiceKey{hook_key, std::string{name}});
  if (iter != service_map->end()) {
    service_fd = iter->second.instance->getRunningFd(hook_key.hook);
  }
  return service_fd;
}

/**
 * @brief 更新 map 元素
 *
 * @param hook_key 键
 * @param prog 程序
 * @param hash_fd 哈希表 fd
 * @param prog_running_fd 运行时服务 fd
 * @return true 成功
 * @return false 失败
 */
bool PinnedProgArray::updateMapElem(const daemon::HookKey &hook_key,
                                    const daemon::ConfigEbpfPtr &prog, int hash_fd,
                                    int prog_running_fd) {

  if (hook_key.hook == daemon::HookType::UNKNOWN) {
    return false;
  }
  if (prog == nullptr) {
    return false;
  }
  if (hash_fd <= 0) {
    return false;
  }
  if (prog_running_fd <= 0) {
    return false;
  }

  int array_fd = getPinnedFd();
  if (array_fd == FD_INVALID) {
    return false;
  }

  auto prog_id = prog->getProgId();
  __u32 index{};
  __u64 key = CHAIN_KEY(hook_key.hook, hook_key.hook_id, prog_id);
  if (bpf_map_lookup_elem(hash_fd, &key, &index) != 0) {
    LOG(warn, "Program '{}' (prog_id {}) not in current chain, cannot insert", prog->getName(),
        prog_id);
    return false;
  }

  // 注意：即使写入 prog_fd（比如 1），但 bpftool map dump 的时候可能不是这个值
  //       原因是 eBPF 机制会找到这个 fd 对应的 eBPF 程序的程序 ID
  //       这个程序 ID 就是 bpftool prog show <PROG> 看到的 prog_id
  auto ret = bpf_map_update_elem(array_fd, &index, &prog_running_fd, BPF_ANY);
  if (ret != 0) {
    LOG(error, "Failed to insert prog fd into prog_array at index {} (code {}: {})", index, -ret,
        strerror(-ret));
    return false;
  }

  LOG(info,
      "Inserted prog '{}' (prog_id {}) at index {}, value (program fd {}) is what the \"bpftool "
      "prog "
      "show "
      "<PROG>\" prog_id is",
      prog->getName(), prog_id, index, prog_running_fd);
  return true;
}

} // namespace ebpf
} // namespace hebpf
