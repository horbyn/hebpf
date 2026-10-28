// clang-format off
#include "pinned_hash.h"
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
bool PinnedHash::init() {
  if (initialized_.load()) {
    return true;
  }

  if (!createPinnedInfo(MAP_NAME_ID_TO_INDEX, MAX_CHAIN_PROGS)) {
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
nlohmann::json PinnedHash::getDebugStatus() {
  std::vector<EbpfHash> result{};

  auto hash_fd = getPinnedFd();
  ASSERT(hash_fd >= 0);

  __u64 key64{}, next_key64{};
  std::vector<EbpfHashElem> hash{};
  while (bpf_map_get_next_key(hash_fd, &key64, &next_key64) == 0) {
    __u32 index{};
    if (bpf_map_lookup_elem(hash_fd, &next_key64, &index) == 0) {
      hash.push_back(EbpfHashElem{fmt::format("{:#x}", next_key64), index});
    }
    key64 = next_key64;
  } // end while(hash)

  result.emplace_back(getPinnedPath(), hash);

  return result;
}

/**
 * @brief 更新 map 元素
 *
 * @param hook_key 键
 * @param start_index 起始索引
 * @param progs 程序数组（失败的程序会从中移除）
 * @return true 成功
 * @return false 失败
 */
bool PinnedHash::updateMapElem(const daemon::HookKey &hook_key, int start_index,
                               daemon::ProgList progs) {
  if (!initialized_.load()) {
    LOG(error, "Object not initialized");
    return false;
  }

  ASSERT(start_index >= 0);
  if (start_index >= MAX_CHAIN_PROGS) {
    LOG(warn, "{} programs exceeded", MAX_CHAIN_PROGS);
    return false;
  }

  int hash_fd = getPinnedFd();
  auto index = start_index;
  for (auto iter = progs.begin(); iter != progs.end();) {
    ASSERT(*iter != nullptr);

    auto prog_id = (*iter)->getProgId();
    __u64 key = CHAIN_KEY(hook_key.hook, hook_key.hook_id, prog_id);
    auto ret = bpf_map_update_elem(hash_fd, &key, &index, BPF_ANY);
    if (ret != 0) {
      LOG(error, "Failed to update id_to_index for id {} -> index {} (code {}: {})", prog_id, index,
          -ret, strerror(-ret));
      iter = progs.erase(iter); // 将失败的程序从中移除出去 TODO: 真的能移动出去吗? 这里是副本
      continue;
    }
    LOG(debug, "Chain mapping: {{ prog_id:{}  hook_id:{}  hook_type:{} }} -> {{ index:{} }}",
        prog_id, hook_key.hook_id, enumName(hook_key.hook), index);
    ++index;
    ++iter;

  } // end for(progs)

  cached_.emplace(hook_key, progs);

  return true;
}

/**
 * @brief 清理 pinned map
 * @param hook_key 键
 *
 * @return true 成功
 * @return false 失败
 */
bool PinnedHash::cleanMap(daemon::HookKey hook_key) {
  int hash_fd = getPinnedFd();
  if (hash_fd == FD_INVALID) {
    LOG(error, "Cannot clean pinned map {} of id hash", enumName(hook_key.hook));
    return false;
  }

  auto iter = cached_.find(hook_key);
  if (iter == cached_.end()) {
    LOG(error, "Cannot clean pinned map {} of id hash, not found", enumName(hook_key.hook));
    return false;
  }

  std::vector<__u64> keys_to_del{};
  for (const auto &prog : iter->second) {
    __u64 key = CHAIN_KEY(hook_key.hook, hook_key.hook_id, prog->getProgId());
    keys_to_del.push_back(key);
  }

  __u64 key = 0;
  __u64 next_key{};
  while (bpf_map_get_next_key(hash_fd, &key, &next_key) == 0) {
    auto iter_find_vec = std::find_if(keys_to_del.begin(), keys_to_del.end(),
                                      [&next_key](const __u64 &k) { return k == next_key; });
    if (iter_find_vec != keys_to_del.end()) {
      // 仅删除这个 key 的 kv 对
      bpf_map_delete_elem(hash_fd, &next_key);
    }
    key = next_key;
  }
  cached_.erase(hook_key);
  return true;
}

/**
 * @brief 回滚
 *
 * @param current_map 当前 HookMap
 * @return true 回滚成功
 * @return false 回滚失败
 */
bool PinnedHash::rollback(const daemon::HookMap &current_map) {
  auto diff = getDiff(current_map);

  auto result = true; // 回滚没有出错
  for (const auto &[hook_key, _] : diff) {
    if (!cleanMap(hook_key)) {
      LOG(warn, "Rollback pinned map HASH with key {} failed", enumName(hook_key.hook));
      result = false;
    }
  }
  return result;
}

/**
 * @brief 获取 map 类型
 *
 * @return bpf_map_type 类型
 */
bpf_map_type PinnedHash::getMapType() const { return BPF_MAP_TYPE_HASH; }

/**
 * @brief 获取 key 大小
 *
 * @return size_t 大小
 */
size_t PinnedHash::getKeySize() const { return sizeof(__u64); }

/**
 * @brief 获取 value 大小
 *
 * @return size_t 大小
 */
size_t PinnedHash::getValueSize() const { return sizeof(__u32); }

/**
 * @brief 获取两个缓存的差值
 * @note 需要保证已经缓存的内容多于当前内容
 *
 * @param current_map 当前内容
 * @return daemon::HookMap 已经缓存的内容与当前内容之间的差值
 */
daemon::HookMap PinnedHash::getDiff(const daemon::HookMap &current_map) const {
  ASSERT(cached_.size() >= current_map.size());

  daemon::HookMap diff{};
  // cached_ 和 current_map 有三种情况
  // 1. cached_ 有，current 没有
  // 2. cached_ 没有，current 有（不考虑这种情况）
  // 3. cached_ 和 current_map 都有
  for (const auto &[hook_key, prog_list] : cached_) {
    if (current_map.find(hook_key) == current_map.end()) {
      diff.emplace(hook_key, prog_list);
    }
  } // end for()

  return diff;
}

} // namespace ebpf
} // namespace hebpf
