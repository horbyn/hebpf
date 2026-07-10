// clang-format off
#include "loki_streams.h"
// clang-format on

namespace hebpf {
namespace loki {

LokiStreams::LokiStreams(const std::vector<LokiStreamEntry> &streams) : streams{streams} {}

/**
 * @brief 追加一对标签日志记录到 loki streams
 *
 * @param stream 标签对
 */
void LokiStreams::appendStream(const LokiStreamEntry &stream) { streams.push_back(stream); }

/**
 * @brief 获取 loki streams
 *
 * @return std::vector<LokiStreamEntry> 标签日志记录
 */
std::vector<LokiStreamEntry> LokiStreams::getStreams() const { return streams; }

} // namespace loki
} // namespace hebpf
