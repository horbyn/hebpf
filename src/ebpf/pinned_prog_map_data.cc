// clang-format off
#include "pinned_prog_map_data.h"
// clang-format on

namespace hebpf {
namespace ebpf {

EbpfHashElem::EbpfHashElem(std::string_view key, uint32_t value)
    : key{std::string{key}}, value{value} {}

EbpfHash::EbpfHash(std::string_view map_path, const std::vector<EbpfHashElem> &data)
    : map_path{std::string{map_path}}, hash{data} {}

EbpfArray::EbpfArray(std::string_view map_path, const std::vector<uint32_t> &data)
    : map_path{std::string{map_path}}, array{data} {}

EbpfProgMap::EbpfProgMap(const std::vector<EbpfHash> &hashes, const std::vector<EbpfArray> &arrays)
    : hashes{hashes}, arrays{arrays} {}

/**
 * @brief 追加一个哈希表
 *
 * @param hash 哈希表
 */
void EbpfProgMap::appendHash(const EbpfHash &hash) { hashes.push_back(hash); }

/**
 * @brief 追加一个数组
 *
 * @param array 数组
 */
void EbpfProgMap::appendArray(const EbpfArray &array) { arrays.push_back(array); }

} // namespace ebpf
} // namespace hebpf
