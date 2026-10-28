// clang-format off
#include "event_holder.h"
// clang-format on

namespace hebpf {
namespace event {

EventBusHolder::EventBusHolder(std::weak_ptr<EventBusIf> event_bus) : bus_{event_bus} {}

} // namespace event
} // namespace hebpf
