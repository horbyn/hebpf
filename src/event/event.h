#pragma once

// clang-format off
#include <memory>
#include "src/daemon/hooks.h"
#include "src/ebpf/region_map.h"
#include "src/ebpf/service_map.h"
// clang-format on

class Region;

namespace hebpf {
namespace event {

class EventIf {
public:
  explicit EventIf(std::shared_ptr<daemon::HookMap> map);
  virtual ~EventIf() = default;

  void setMap(std::shared_ptr<daemon::HookMap> map);
  std::shared_ptr<daemon::HookMap> getMap() const;

  void setDiff(std::shared_ptr<std::vector<daemon::Diff>> diff);
  std::shared_ptr<std::vector<daemon::Diff>> getDiff() const;

protected:
  std::shared_ptr<daemon::HookMap> map_;
  std::shared_ptr<std::vector<daemon::Diff>> diff_;
};

class ConfigEvent final : public EventIf {
public:
  explicit ConfigEvent(std::shared_ptr<daemon::HookMap> map,
                       std::shared_ptr<std::vector<daemon::Diff>> diff);
};

class RegionEvent final : public EventIf {
public:
  explicit RegionEvent(std::shared_ptr<daemon::HookMap> hook_map,
                       std::shared_ptr<ebpf::RegionMap> region_map,
                       std::shared_ptr<std::vector<daemon::Diff>> diff);
  std::shared_ptr<ebpf::RegionMap> getRegionMap() const;

private:
  std::shared_ptr<ebpf::RegionMap> region_;
};

class RegionFailureEvent final : public EventIf {
public:
  explicit RegionFailureEvent(std::shared_ptr<daemon::HookMap> map,
                              std::shared_ptr<std::vector<daemon::Diff>> diff);
};

class HashEvent final : public EventIf {
public:
  explicit HashEvent(std::shared_ptr<daemon::HookMap> hook_map,
                     std::shared_ptr<std::vector<daemon::Diff>> diff);
};

class HashFailureEvent final : public EventIf {
public:
  explicit HashFailureEvent(std::shared_ptr<daemon::HookMap> map,
                            std::shared_ptr<std::vector<daemon::Diff>> diff);
};

class EbpfEvent final : public EventIf {
public:
  explicit EbpfEvent(std::shared_ptr<daemon::HookMap> hook_map,
                     std::shared_ptr<std::vector<daemon::Diff>> diff,
                     std::shared_ptr<ebpf::ServiceMap> ebpf_map);

  std::shared_ptr<ebpf::ServiceMap> getServiceMap() const;

private:
  std::shared_ptr<ebpf::ServiceMap> service_map_;
};

class EbpfFailureEvent final : public EventIf {
public:
  explicit EbpfFailureEvent(std::shared_ptr<daemon::HookMap> map);
};

class ProgArrayFailureEvent final : public EventIf {
public:
  explicit ProgArrayFailureEvent(std::shared_ptr<daemon::HookMap> map,
                                 std::shared_ptr<std::vector<daemon::Diff>> diff);
};

class CompleteEvent final : public EventIf {
public:
  explicit CompleteEvent(std::shared_ptr<daemon::HookMap> map);
};

} // namespace event
} // namespace hebpf
