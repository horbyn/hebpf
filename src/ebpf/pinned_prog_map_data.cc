// clang-format off
#include "pinned_prog_map_data.h"
// clang-format on

namespace hebpf {
namespace ebpf {

EbpfHashElem::EbpfHashElem(std::string_view key, uint32_t value)
    : key{std::string{key}}, value{value} {}

EbpfHash::EbpfHash(const std::vector<EbpfHashElem> &data) : hash{data} {}

EbpfArray::EbpfArray(const std::vector<uint32_t> &data) : array{data} {}

} // namespace ebpf
} // namespace hebpf
