#pragma once

// clang-format off
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {
namespace loki {

using LokiStream = std::map<std::string, std::string>;
using LokiValues = std::vector<std::pair<std::string, std::string>>;

struct LokiStreamEntry {
  LokiStream stream; // 标签
  LokiValues values; // [时间戳, 日志行]
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(LokiStreamEntry, stream, values);

class LokiStreams final {
public:
  explicit LokiStreams() = default;
  explicit LokiStreams(const std::vector<LokiStreamEntry> &streams);

  void appendStream(const LokiStreamEntry &stream);
  std::vector<LokiStreamEntry> getStreams() const;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(LokiStreams, streams);

private:
  std::vector<LokiStreamEntry> streams;
};

} // namespace loki
} // namespace hebpf
