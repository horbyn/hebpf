#pragma once

// clang-format off
#include <string>
#include <string_view>
#include <vector>
#include "nlohmann/json.hpp"
// clang-format on

namespace hebpf {
namespace ebpf {

class EbpfHashElem {
public:
  explicit EbpfHashElem(std::string_view key, uint32_t value);
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(EbpfHashElem, key, value);

private:
  std::string key;
  uint32_t value;
};

class EbpfHash {
public:
  explicit EbpfHash(const std::vector<EbpfHashElem> &data);
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(EbpfHash, hash);

private:
  std::vector<EbpfHashElem> hash;
};

class EbpfArray {
public:
  explicit EbpfArray(const std::vector<uint32_t> &data);
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(EbpfArray, array);

private:
  std::vector<uint32_t> array;
};

} // namespace ebpf
} // namespace hebpf
