// clang-format off
#include "event.h"
// clang-format on

namespace hebpf {
namespace event {

EventIf::EventIf(std::shared_ptr<daemon::HookMap> map) : map_(map) {}

/**
 * @brief 设置 HookMap 对象
 *
 * @param map HookMap 对象
 */
void EventIf::setMap(std::shared_ptr<daemon::HookMap> map) { map_ = map; }

/**
 * @brief 获取 HookMap 对象
 *
 * @return std::shared_ptr<daemon::HookMap> 共享指针形式
 */
std::shared_ptr<daemon::HookMap> hebpf::event::EventIf::getMap() const { return map_; }

/**
 * @brief 设置 Diff 对象
 *
 * @param diff 对象
 */
void EventIf::setDiff(std::shared_ptr<std::vector<daemon::Diff>> diff) { diff_ = diff; }

/**
 * @brief 获取 Diff 对象
 *
 * @return std::shared_ptr<std::vector<daemon::Diff>> 共享指针形式
 */
std::shared_ptr<std::vector<daemon::Diff>> EventIf::getDiff() const { return diff_; }

ConfigEvent::ConfigEvent(std::shared_ptr<daemon::HookMap> map,
                         std::shared_ptr<std::vector<daemon::Diff>> diff)
    : EventIf(map) {
  setDiff(diff);
}

RegionEvent::RegionEvent(std::shared_ptr<daemon::HookMap> hook_map,
                         std::shared_ptr<ebpf::RegionMap> region_map,
                         std::shared_ptr<std::vector<daemon::Diff>> diff)
    : EventIf(hook_map), region_{region_map} {
  setDiff(diff);
}

/**
 * @brief 获取 RegionMap 对象
 *
 * @return std::shared_ptr<ebpf::RegionMap> 指针
 */
std::shared_ptr<ebpf::RegionMap> RegionEvent::getRegionMap() const { return region_; }

RegionFailureEvent::RegionFailureEvent(std::shared_ptr<daemon::HookMap> map,
                                       std::shared_ptr<std::vector<daemon::Diff>> diff)
    : EventIf(map) {
  setDiff(diff);
}

HashEvent::HashEvent(std::shared_ptr<daemon::HookMap> hook_map,
                     std::shared_ptr<std::vector<daemon::Diff>> diff)
    : EventIf(hook_map) {
  setDiff(diff);
}

HashFailureEvent::HashFailureEvent(std::shared_ptr<daemon::HookMap> map,
                                   std::shared_ptr<std::vector<daemon::Diff>> diff)
    : EventIf(map) {
  setDiff(diff);
}

EbpfEvent::EbpfEvent(std::shared_ptr<daemon::HookMap> hook_map,
                     std::shared_ptr<std::vector<daemon::Diff>> diff,
                     std::shared_ptr<ebpf::ServiceMap> ebpf_map)
    : EventIf(hook_map), service_map_{ebpf_map} {
  setDiff(diff);
}

/**
 * @brief 获取服务列表
 *
 * @return std::shared_ptr<ebpf::ServiceMap> 服务列表
 */
std::shared_ptr<ebpf::ServiceMap> EbpfEvent::getServiceMap() const { return service_map_; }

EbpfFailureEvent::EbpfFailureEvent(std::shared_ptr<daemon::HookMap> map) : EventIf(map) {}

ProgArrayFailureEvent::ProgArrayFailureEvent(std::shared_ptr<daemon::HookMap> map,
                                             std::shared_ptr<std::vector<daemon::Diff>> diff)
    : EventIf(map) {
  setDiff(diff);
}

CompleteEvent::CompleteEvent(std::shared_ptr<daemon::HookMap> map) : EventIf(map) {}

} // namespace event
} // namespace hebpf
