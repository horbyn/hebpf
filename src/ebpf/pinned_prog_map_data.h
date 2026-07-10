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
  explicit EbpfHashElem() = default;
  explicit EbpfHashElem(std::string_view key, uint32_t value);
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(EbpfHashElem, key, value);

private:
  std::string key;
  uint32_t value;
};

class EbpfHash {
public:
  explicit EbpfHash() = default;
  explicit EbpfHash(std::string_view map_path, const std::vector<EbpfHashElem> &data);
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(EbpfHash, map_path, hash);

private:
  std::string map_path;
  std::vector<EbpfHashElem> hash;
};

class EbpfArray {
public:
  explicit EbpfArray() = default;
  explicit EbpfArray(std::string_view map_path, const std::vector<uint32_t> &data);
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(EbpfArray, map_path, array);

private:
  std::string map_path;
  std::vector<uint32_t> array;
};

class EbpfProgMap final {
public:
  explicit EbpfProgMap() = default;
  explicit EbpfProgMap(const std::vector<EbpfHash> &hashes, const std::vector<EbpfArray> &arrays);
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(EbpfProgMap, hashes, arrays);

  void appendHash(const EbpfHash &hash);
  void appendArray(const EbpfArray &array);

private:
  std::vector<EbpfHash> hashes;
  std::vector<EbpfArray> arrays;
};

} // namespace ebpf
} // namespace hebpf
