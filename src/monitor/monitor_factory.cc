// clang-format off
#include "monitor_factory.h"
#include "src/common/assert.h"
#include "monitor.h"
// clang-format on

namespace hebpf {
namespace monitor {

std::shared_ptr<MonitorIf> MonitorFactory::create(std::string_view bind_address) {
  ASSERT(!bind_address.empty());
  return std::make_shared<Monitor>(bind_address);
}

} // namespace monitor
} // namespace hebpf
