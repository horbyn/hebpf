// clang-format off
#include "pinned_prog_map.h"
#include <filesystem>
#include <bpf/bpf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <boost/algorithm/string.hpp>
#include "src/common/assert.h"
#include "src/common/enum_name.hpp"
#include "src/common/exception.h"
#include "src/fd/fd_if.h"
#include "ebpf_common.h"
#include "hebpf_version.h"
#include "pinned_prog_map_data.h"
// clang-format on

namespace hebpf {
namespace ebpf {

// map 名称与内核态定义（src/ebpf/ebpf_chain.h）保持一致

constexpr std::string_view MAP_NAME_PROG_ARRAY_TC = "TC_In_parray";
constexpr std::string_view MAP_NAME_ID_TO_INDEX_TC = "TC_In_idhash";
constexpr std::string_view MAP_NAME_PROG_ARRAY_XDP = "XDP_Gen_parray";
constexpr std::string_view MAP_NAME_ID_TO_INDEX_XDP = "XDP_Gen_idhash";

PinnedProgMap::~PinnedProgMap() { cleanup(); }

/**
 * @brief 初始化公共 map，若已存在则打开，否则创建并 pin
 * @return true 成功，false 失败
 */
bool PinnedProgMap::init() {
  // 确保 bpffs 已挂载
  ensureBpffsMounted();

  // 确保 pin 根目录存在
  struct stat st{};
  if (stat(PATH_BPFFS, &st) != 0) {
    if (mkdir(PATH_BPFFS, 0755) != 0 && errno != EEXIST) {
      LOG(error, "Failed to create program pinned maps{}: {}", PATH_BPFFS, strerror(errno));
      return false;
    }
  }

  // 初始化两种 hook 的 map
  if (!ensureMaps(daemon::HookType::TC)) {
    return false;
  }
  if (!ensureMaps(daemon::HookType::XDP_GENERIC)) {
    return false;
  }

  {
    std::lock_guard<std::mutex> lock1{mutex_}; // 保护自己的私有成员 prog_registry_
    {
      std::lock_guard<std::mutex> lock2{prog_registry_.mtx}; // 保护 prog_registry_ 内部成员 map
      const auto &defaults = getDefaultProgMap();
      for (const auto &[name, info] : defaults) {
        prog_registry_.map[name] = ProgInfo{info.id, info.hook};
      }
    }
  }

  initialized_.store(true);
  return true;
}

/**
 * @brief 清理资源，unpin map
 * @note 在程序退出时调用
 */
void PinnedProgMap::cleanup() {
  {
    std::lock_guard<std::mutex> lock{mutex_};
    for (auto &[hook, maps] : hook_maps_) {
      if (maps.prog_array_fd >= 0) {
        close(maps.prog_array_fd);
        unlink(maps.pin_path_prog_array.c_str());
      }
      if (maps.id_hash_fd >= 0) {
        close(maps.id_hash_fd);
        unlink(maps.pin_path_id_to_index.c_str());
      }
    }
    hook_maps_.clear();
  }
  initialized_.store(false);
}

/**
 * @brief 获取 pinned eBPF map 的数据信息对接 Debug 服务器
 *
 * @return nlohmann::json JSON 对象
 */
nlohmann::json PinnedProgMap::getDebugStatus() {
  nlohmann::json result{};

  {
    std::lock_guard<std::mutex> lock{mutex_};
    for (const auto &[hook, maps] : hook_maps_) {
      auto hash_fd = maps.id_hash_fd;
      std::vector<EbpfHashElem> hash{};
      __u64 key64{}, next_key64{};

      while (bpf_map_get_next_key(hash_fd, &key64, &next_key64) == 0) {
        __u32 index{};
        if (bpf_map_lookup_elem(hash_fd, &next_key64, &index) == 0) {
          hash.push_back(EbpfHashElem{fmt::format("{:#x}", next_key64), index});
        }
        key64 = next_key64;
      } // end while(hash)

      EbpfHash ebpf_hash{hash};
      result[maps.pin_path_id_to_index] = ebpf_hash;

      auto array_fd = maps.prog_array_fd;
      std::vector<uint32_t> array{};
      for (__u32 key = 0; key < MAX_CHAIN_PROGS; ++key) {
        __u32 index{};
        if (bpf_map_lookup_elem(array_fd, &key, &index) == 0) {
          if (index != 0) {
            array.push_back(index);
          }
        }
      } // end for(array)

      EbpfArray ebpf_array{array};
      result[maps.pin_path_prog_array] = ebpf_array;
    } // end for()
  }

  return result;
}

/**
 * @brief 设置程序数组中网卡索引对应的区域
 *
 * @param hook_type 绑定类型
 * @param ifindex 网卡索引
 * @return int 数组起始下标；出错返回 -1
 */
int PinnedProgMap::allocateRegion(daemon::HookType hook_type, int ifindex) {
  ASSERT(ifindex > 0);

  std::lock_guard<std::mutex> lock{mutex_};
  auto &hook_regions = regions_[hook_type]; // 不存在就创建
  if (hook_regions.find(ifindex) != hook_regions.end()) {
    return hook_regions[ifindex].start_index;
  }

  // 遍历当前 hook 类型对应的整个哈希表
  int next_start = 0;
  for (const auto &[_, region] : hook_regions) {
    if (region.start_index >= next_start) {
      next_start = region.start_index + MAX_PER_NIC_PROGS;
    }
  } // end for()
  if (next_start + MAX_PER_NIC_PROGS > MAX_CHAIN_PROGS) {
    LOG(error, "No space in prog_array for hook {} ifindex {}", enumName(hook_type), ifindex);
    return -1;
  }
  hook_regions[ifindex] = {next_start};
  LOG(debug, "Allocating region, base {} for hook {} ifindex {}", next_start, enumName(hook_type),
      ifindex);
  return next_start;
}

/**
 * @brief 释放网卡对应的区域
 *
 * @param hook_type 绑定类型
 * @param ifindex 网卡索引
 */
void PinnedProgMap::releaseRegion(daemon::HookType hook_type, int ifindex) {
  std::lock_guard<std::mutex> lock{mutex_};
  regions_[hook_type].erase(ifindex);
}

/**
 * @brief 获取某个绑定类型下某个网卡在程序数组区域的起始下标
 *
 * @param hook_type 绑定类型
 * @param ifindex 网卡索引
 * @return int 起始下标，出错返回 -1
 */
int PinnedProgMap::getStartIndex(daemon::HookType hook_type, int ifindex) const {
  std::lock_guard<std::mutex> lock{mutex_};
  auto iter = regions_.find(hook_type);
  if (iter == regions_.end()) {
    return -1;
  }
  auto iter2 = iter->second.find(ifindex);
  if (iter2 == iter->second.end()) {
    return -1;
  }
  return iter2->second.start_index;
}

/**
 * @brief 根据 hook 类型和程序名列表更新尾调用链顺序
 *
 * @note 仅更新 id hash
 * @param hook_type Hook 类型 (TC_Ingress 或 XDP_Generic)
 * @param ordered_names 按配置顺序排列的 eBPF 程序名称与网卡索引列表
 * @return true 成功
 */
bool PinnedProgMap::updateChain(daemon::HookType hook_type, const NamesIfindexVec &ordered_names) {
  if (!initialized_.load()) {
    LOG(error, "Object not initialized");
    return false;
  }

  decltype(hook_maps_.begin()) iter{};
  int hash_fd = FD_INVALID;
  {
    std::lock_guard<std::mutex> lock{mutex_};
    iter = hook_maps_.find(hook_type);
    if (iter == hook_maps_.end()) {
      LOG(error, "Hook type {} not prepared", enumName(hook_type));
      return false;
    }

    hash_fd = iter->second.id_hash_fd;
  }

  __u64 key = 0;
  __u64 next_key{};
  while (bpf_map_get_next_key(hash_fd, &key, &next_key) == 0) {
    bpf_map_delete_elem(hash_fd, &next_key);
    key = next_key;
  }

  // 现在的 ordered_names 是同一个 hook 的所有配置的顺序，如
  //     [{"程序官方名称", 网卡索引}, {"lb_xdpgen", 1}, {"acl_tc", 2}, {"acl_xdpgen", 1}]
  // 需要按照同一个 hook 不同网卡单独排序，如
  //     {1, ["lb_xdpgen", "acl_xdpgen"]}
  //     {2, ["acl_tc"]}
  std::unordered_map<int, std::vector<std::string>> ifindex_orders{};
  for (const auto &elem : ordered_names) {
    auto name = elem.first;
    auto ifindex = elem.second;
    ifindex_orders[ifindex].emplace_back(name);
  }

  for (auto &[ifindex, names] : ifindex_orders) {
    int index = getStartIndex(hook_type, ifindex);
    ASSERT(index >= 0);
    if (index >= MAX_PER_NIC_PROGS) {
      // 每个网卡最多绑定 MAX_PER_NIC_PROGS 个 eBPF 程序
      continue;
    }

    // 第一个元素加入 Scheduler
    names.emplace(names.begin(),
                  getCanonicalName(hook_type, ebpf::PinnedProgMap::UnofficialName::LIBRARY,
                                   hook_type == daemon::HookType::TC
                                       ? std::string{SCHEDULER_TC_LIB}
                                       : std::string{SCHEDULER_XDP_LIB}));

    for (const auto &name : names) {
      int id{};
      daemon::HookType hook{};
      if (!lookupProgInfo(name, id, hook)) {
        LOG(warn, "Program name '{}' not found in ID Hash Map, skipped", name);
        continue;
      }

      if (hook != hook_type) {
        continue;
      }

      __u64 key = CHAIN_KEY(ifindex, id);
      if (bpf_map_update_elem(hash_fd, &key, &index, BPF_ANY) != 0) {
        LOG(error, "Failed to update id_to_index for id {} -> index {}", id, index);
        return false;
      }
      LOG(debug, "Chain mapping: id {} -> index {}", id, index);
      ++index;
    } // end for(vec)

  } // end for(hash)
  LOG(debug, "To display all the elements in the map, execute \"bpftool map dump name [{} / {}]\"",
      MAP_NAME_ID_TO_INDEX_TC, MAP_NAME_ID_TO_INDEX_XDP);

  return true;
}

/**
 * @brief 将指定程序的 fd 插入到 prog_array 的正确位置（由内部映射决定）
 *
 * @note 更新程序数组
 * @param hook_type Hook 类型
 * @param name 程序名称（用于获取 ID）
 * @param prog_fd 程序 fd
 * @param ifindex 网卡索引
 * @return true 成功
 */
bool PinnedProgMap::insertProg(daemon::HookType hook_type, std::string_view name, int prog_fd,
                               int ifindex) {
  ASSERT(prog_fd >= 0);
  if (!initialized_.load()) {
    LOG(error, "Object not initialized");
    return false;
  }

  decltype(hook_maps_.begin()) iter{};
  int hash_fd{}, array_fd{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    iter = hook_maps_.find(hook_type);
    if (iter == hook_maps_.end()) {
      LOG(error, "Hook type {} not prepared", enumName(hook_type));
      return false;
    }

    hash_fd = iter->second.id_hash_fd;
    array_fd = iter->second.prog_array_fd;
  }

  int id{};
  daemon::HookType hook{};
  if (!lookupProgInfo(name, id, hook)) {
    LOG(error, "Program name '{}' not found in ebpf_id.map", name);
    return false;
  }
  if (hook != hook_type) {
    LOG(error, "Program '{}' hook type mismatch", name);
    return false;
  }

  __u32 index{};
  __u64 key = CHAIN_KEY(ifindex, id);
  if (bpf_map_lookup_elem(hash_fd, &key, &index) != 0) {
    LOG(warn, "Program '{}' (id {}) not in current chain, cannot insert", name, id);
    return false;
  }

  // 注意：即使写入 prog_fd（比如 1），但 bpftool map dump 的时候可能不是这个值
  //       原因是 eBPF 机制会找到这个 fd 对应的 eBPF 程序的程序 ID
  //       这个程序 ID 就是 bpftool prog show <PROG> 看到的 id
  if (bpf_map_update_elem(array_fd, &index, &prog_fd, BPF_ANY) != 0) {
    LOG(error, "Failed to insert prog fd into prog_array at index {}", index);
    return false;
  }

  LOG(info,
      "Inserted prog '{}' (id {}) at index {}, value (program fd) is what the \"bpftool prog show "
      "<PROG>\" id is",
      name, id, index);
  return true;
}

/**
 * @brief 查找程序名对应的 ID 和 hook 类型
 *
 * @param name 官方程序名
 * @param id 唯一标识（返回值）
 * @param hook 绑定类型（返回值）
 * @return true 查找成功
 * @return false 查找失败
 */
bool PinnedProgMap::lookupProgInfo(std::string_view name, int &id, daemon::HookType &hook) const {
  std::lock_guard<std::mutex> lock1{mutex_};
  std::lock_guard<std::mutex> lock2{prog_registry_.mtx};
  auto it = prog_registry_.map.find(std::string{name});
  if (it == prog_registry_.map.end()) {
    return false;
  }
  id = it->second.id;
  hook = it->second.hook;
  return true;
}

/**
 * @brief 生成官方名称
 *
 * @note 如输入 name 为 acl，绑定点类型为 XDP_GEN，则返回 "ACL_XDP_GEN"
 *
 * @param hook_type 绑定点类型
 * @param type 非官方名称类型
 * @param name 项目内部名称
 * @return std::string 官方名称
 */
std::string PinnedProgMap::getCanonicalName(daemon::HookType hook_type, UnofficialName type,
                                            std::string_view name) {
  ASSERT(hook_type != daemon::HookType::UNKNOWN);
  ASSERT(!name.empty());

  std::string result{};

  std::string path = std::string{name};
  if (type == UnofficialName::LIBRARY) {
    namespace fs = std::filesystem;
    using boost::algorithm::ends_with;
    using boost::algorithm::starts_with;

    // 移除动态库前面的路径，如 /path/to/libxxx.so，只保留 libxxx.so
    std::string filename = fs::path(path).filename().string();

    if (ends_with(filename, ".so")) {
      filename = filename.substr(0, filename.size() - 3);
    }

    const std::string prefix = "lib" HEBPF_PROJECT "_";
    if (!starts_with(filename, prefix)) {
      GLOBAL_LOG(warn,
                 "Invalid library name \"{}\", expected prefix \"{}\", using this invalid name as "
                 "official name",
                 path, prefix);
      return path;
    }
    path = filename.substr(prefix.size());

    // 取下划线之前的部分，如 XDP 动态库命名是 libxxx_xdp.so
    size_t underscore_pos = path.find('_');
    if (underscore_pos != std::string::npos) {
      path = path.substr(0, underscore_pos);
    }
  }

  std::transform(path.begin(), path.end(), std::back_inserter(result), ::toupper);
  result = result + "_" + std::string{enumName(hook_type)};

  return result;
}

/**
 * @brief 获取或创建指定 hook 的 map
 *
 * @param hook_type 类型
 * @return true 成功
 * @return false 失败
 */
bool PinnedProgMap::ensureMaps(daemon::HookType hook_type) {
  {
    std::lock_guard<std::mutex> lock{mutex_};
    if (hook_maps_.find(hook_type) != hook_maps_.end()) {
      return true;
    }
  }

  HookMaps maps{};
  std::string prog_array_name{};
  std::string id_to_index_name{};

  switch (hook_type) {
  case daemon::HookType::TC:
    prog_array_name = MAP_NAME_PROG_ARRAY_TC;
    id_to_index_name = MAP_NAME_ID_TO_INDEX_TC;
    break;
  case daemon::HookType::XDP_GENERIC:
    prog_array_name = MAP_NAME_PROG_ARRAY_XDP;
    id_to_index_name = MAP_NAME_ID_TO_INDEX_XDP;
    break;
  default:
    LOG(error, "Unsupported hook type for chain");
    return false;
  }

  maps.pin_path_prog_array = getPinPath(prog_array_name);
  maps.pin_path_id_to_index = getPinPath(id_to_index_name);

  // 尝试打开已 pin 的 map
  maps.prog_array_fd = bpf_obj_get(maps.pin_path_prog_array.c_str());
  maps.id_hash_fd = bpf_obj_get(maps.pin_path_id_to_index.c_str());

  if (maps.prog_array_fd < 0 || maps.id_hash_fd < 0) {
    if (maps.prog_array_fd >= 0) {
      close(maps.prog_array_fd);
    }
    if (maps.id_hash_fd >= 0) {
      close(maps.id_hash_fd);
    }

    maps.prog_array_fd = bpf_map_create(BPF_MAP_TYPE_PROG_ARRAY, prog_array_name.data(),
                                        sizeof(__u32), sizeof(__u32), MAX_CHAIN_PROGS, nullptr);
    if (maps.prog_array_fd < 0) {
      LOG(error, "Failed to create prog_array map {}: {}", prog_array_name, strerror(errno));
      return false;
    }

    maps.id_hash_fd = bpf_map_create(BPF_MAP_TYPE_HASH, id_to_index_name.data(), sizeof(__u64),
                                     sizeof(__u32), MAX_CHAIN_PROGS, nullptr);
    if (maps.id_hash_fd < 0) {
      close(maps.prog_array_fd);
      LOG(error, "Failed to create id_to_index map {}: {}", id_to_index_name, strerror(errno));
      return false;
    }

    if (bpf_obj_pin(maps.prog_array_fd, maps.pin_path_prog_array.c_str()) < 0) {
      LOG(error, "Failed to pin {}: {}", prog_array_name, strerror(errno));
      close(maps.prog_array_fd);
      close(maps.id_hash_fd);
      return false;
    }
    if (bpf_obj_pin(maps.id_hash_fd, maps.pin_path_id_to_index.c_str()) < 0) {
      LOG(error, "Failed to pin {}: {}", id_to_index_name, strerror(errno));
      unlink(maps.pin_path_prog_array.c_str());
      close(maps.prog_array_fd);
      close(maps.id_hash_fd);
      return false;
    }
  }

  {
    std::lock_guard<std::mutex> lock{mutex_};
    hook_maps_[hook_type] = maps;
  }
  LOG(info, "Initialized for hook {}", enumName(hook_type));
  return true;
}

/**
 * @brief 获取 pinned map 路径
 *
 * @param map_name eBPF map 名称
 * @return std::string pinned 路径
 */
std::string PinnedProgMap::getPinPath(std::string_view map_name) {
  return std::string{PATH_BPFFS} + std::string{map_name};
}

} // namespace ebpf
} // namespace hebpf
