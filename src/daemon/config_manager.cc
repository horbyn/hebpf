// clang-format off
#include "config_manager.h"
#include <unordered_set>
#include "src/common/assert.h"
#include "src/common/common.h"
#include "src/common/enum_name.hpp"
#include "src/common/exception.h"
#include "src/event/event_bus.h"
#include "generated/ebpf_prog_map.h"
#include "configs.hpp"
// clang-format on

namespace hebpf {
namespace daemon {

ConfigManager::ConfigManager(std::weak_ptr<event::EventBusIf> event_bus)
    : event::EventBusHolder{event_bus} {
  auto bus = bus_.lock();
  if (bus == nullptr) {
    throw EXCEPT("event bus is null");
  }
  bus->subscribe<event::RegionFailureEvent>(event::EventHandler{
      FUNCTION_LINE, [this](const event::EventIf &eve) { onRegionFailure(eve); }});
  bus->subscribe<event::CompleteEvent>(
      event::EventHandler{FUNCTION_LINE, [this](const event::EventIf &eve) { onComplete(eve); }});
}

ConfigManager::~ConfigManager() { maps_.clear(); }

/**
 * @brief 更新配置列表
 *
 * @param new_configs 新表
 */
void ConfigManager::update(const Configs::EbpfMap &new_configs) {
  auto diff = std::make_shared<std::vector<Diff>>();
  auto new_map = computeDiff(new_configs, diff);
  if (new_map == nullptr || (new_map != nullptr && new_map->empty())) {
    return;
  }

  std::shared_ptr<event::EventBusIf> bus{};
  {
    std::lock_guard<std::mutex> lock{mutex_};
    bus = bus_.lock();
    if (bus == nullptr) {
      throw EXCEPT("Event bus is invalid");
    }
  }

  bus->publish(event::ConfigEvent{std::move(new_map), diff});
}

/**
 * @brief 更新调试接口配置
 *
 * @param out 配置文件
 */
nlohmann::json ConfigManager::getDebugStatus() const { return maps_; }

/**
 * @brief 缓存当前成功的配置
 *
 * @param hook_map 配置列表
 */
void ConfigManager::cacheCurrent(const HookMap &hook_map) {
  {
    std::lock_guard<std::mutex> lock{mutex_};
    maps_ = hook_map; // 拷贝一份
  }
}

/**
 * @brief Region 事件更新失败
 * @note Path: ConfigManager > Region
 * @note       ConfigManager < Region（失败）
 *
 * @param event 事件对象
 */
void ConfigManager::onRegionFailure(const event::EventIf &event) {
  // Region 会返回成功的 HookMap 和 Diff 列表，将成功的部分缓存到私有成员
  cacheCurrent(*(event.getMap()));
}

/**
 * @brief 所有事件更新完成
 *
 * @param event 事件对象
 */
void ConfigManager::onComplete(const event::EventIf &event) { cacheCurrent(*(event.getMap())); }

/**
 * @brief 计算新旧配置的差异
 *
 * @param new_configs 新配置
 * @param results 差异（返回结果）
 * @return std::shared_ptr<HookMap> 配置对象 EbpfMap 转换 HookMap 对象结果
 */
std::shared_ptr<HookMap>
ConfigManager::computeDiff(const Configs::EbpfMap &new_configs,
                           const std::shared_ptr<std::vector<Diff>> &results) {
  ASSERT(results != nullptr);
  auto new_maps = translate(new_configs);

  // 新配置先加上 scheduler
  for (auto &[hook_key, prog_list] : *new_maps) {
    auto object = makeSchedulerEbpf(hook_key);
    if (object != nullptr) {
      prog_list.emplace(prog_list.begin(), object); // scheduler 插入开头
    }
  }

  results->clear();

  if (maps_.empty()) {
    // 初始化的时候 maps_ 为空，此时所有配置都是加载
    for (auto &kv : *new_maps) {
      const auto &hook_key = kv.first;
      std::transform(kv.second.begin(), kv.second.end(), std::back_inserter(*results),
                     [&hook_key](const auto &prog) { return Diff{DiffType::ADD, hook_key, prog}; });
    }
    return new_maps;
  }

  for (const auto &[hook_key, old_prog_list] : maps_) {
    auto iter = new_maps->find(hook_key);
    if (iter == new_maps->end()) {
      auto cap_key = hook_key;
      std::transform(
          old_prog_list.begin(), old_prog_list.end(), std::back_inserter(*results),
          [&cap_key](const auto &prog) { return Diff{DiffType::REMOVE, cap_key, prog}; });
    } else {
      // TODO: O(N * M) 算法的比较
      for (const auto &old_prog : old_prog_list) {
        auto not_existed = std::none_of(iter->second.begin(), iter->second.end(),
                                        [&old_prog](const auto &ptr) { return *old_prog == *ptr; });
        if (not_existed) {
          // 旧配置在新配置中没有出现 -> 旧配置存在而新配置不存在: 卸载
          results->emplace_back(Diff{DiffType::REMOVE, hook_key, old_prog});
        } else {
          // 旧配置在新配置中存在 -> 新旧配置都存在: 保持
          results->emplace_back(Diff{DiffType::KEEP, hook_key, old_prog});
        }
      } // end for(old_prog_list)

      for (const auto &new_prog : iter->second) {
        if (std::none_of(old_prog_list.begin(), old_prog_list.end(),
                         [&new_prog](const auto &ptr) { return *new_prog == *ptr; })) {
          // 新配置在旧配置中没有出现 -> 旧配置不存在而新配置存在: 加载
          results->emplace_back(Diff{DiffType::ADD, hook_key, new_prog});
        }
      } // end for(new prog list)

    } // end if-else(prog list)
  } // end for(maps_)

  return new_maps;
}

/**
 * @brief 将配置对象转换为 HookMap
 * @note 会检验配置，只有校验通过才会转换
 * @note 程序数组 ProgList 返回时会按照配置顺序保持有序
 *
 * @param new_configs 新配置
 * @return std::shared_ptr<HookMap> 哈希表
 */
std::shared_ptr<HookMap> ConfigManager::translate(const Configs::EbpfMap &new_configs) {
  auto result = std::make_shared<HookMap>();
  const auto &defaults = ebpf::getDefaultProgMap();

  // 检索配置
  for (const auto &[name, conf] : new_configs) {
    auto iter = defaults.find(name);
    if (iter == defaults.end()) {
      LOG(warn, "Configuration name \"{}\" is not match the eBPF program name", name);
      continue;
    }

    conf->setProgId(iter->second.prog_id); // 设置 eBPF 程序 ID
    conf->setName(name);

    const auto &hook = iter->second.hook;
    auto hook_id = conf->getIfindex();
    if (ConfigEbpf::isNetworkHook(hook)) {
      if (hook_id <= 0) {
        // 网络 hook 的 hook_id 表示 NIC 索引，必须大于 0
        LOG(warn, "Invalid hook id {} for network hook", hook_id);
        continue;
      } // end if(hook_id)
    } else {
      hook_id = ID_NONNETWORK_HOOK; // 非网络 hook 总是重置为固定值
    }

    auto &prog_list = (*result)[HookKey{hook, hook_id}]; // 不存在就创建
    prog_list.push_back(conf);

  } // end for(检索配置)

  // 对每个 (hook, hook_id) 的列表按 order 排序
  for (auto &[_, vec] : *result) {
    if (result->size() == 1) {
      continue;
    }
    std::sort(vec.begin(), vec.end(), [](const ConfigEbpfPtr &a, const ConfigEbpfPtr &b) {
      return a->getOrder() < b->getOrder();
    });
  } // end for(排序)

  return result;
}

/**
 * @brief 创建 scheduler 配置对象
 *
 * @param hook_key 键
 * @return ConfigEbpfPtr 配置对象
 */
ConfigEbpfPtr ConfigManager::makeSchedulerEbpf(HookKey hook_key) {
  std::string so_path{};

  // TODO: 将 scheduler 从独立的动态库修改为内部一个 target
  switch (hook_key.hook) {
  case HookType::TC:
    so_path = std::string{SCHEDULER_TC_LIB};
    break;
  case HookType::Sched_Switch:
    so_path = std::string{SCHEDULER_SCHED_SWITCH_LIB};
    break;
  case HookType::XDP_Generic:
    so_path = std::string{SCHEDULER_XDP_LIB};
    break;
  default:
    return nullptr;
  }

  auto name = ConfigEbpf::getSchedulerName(hook_key.hook);
  const auto &defaults = ebpf::getDefaultProgMap();
  auto iter = defaults.find(name);
  if (iter == defaults.end()) {
    return nullptr;
  }

  return std::make_shared<ConfigEbpf>(so_path, "", hook_key.hook_id, 0, iter->second.prog_id, name);
}

} // namespace daemon
} // namespace hebpf
