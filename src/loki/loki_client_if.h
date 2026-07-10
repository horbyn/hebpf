#pragma once

// clang-format off
#include <map>
#include <string>
#include <string_view>
// clang-format on

namespace hebpf {
namespace loki {

class LokiClientIf {
public:
  virtual ~LokiClientIf() = default;

  virtual void push(std::string_view line,
                    const std::map<std::string, std::string> &extra_labels) = 0;
  virtual void flush() = 0;
  virtual void stop() = 0;
};

} // namespace loki
} // namespace hebpf
