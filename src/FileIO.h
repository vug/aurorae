#pragma once
#include <filesystem>
#include <vector>

#include "Utils.h"

namespace std {
enum class byte : unsigned char;
} // namespace std

namespace aur {
// Utility function to read a binary file
template <typename TItem>
std::vector<TItem> readBinaryFile(const std::filesystem::path& filePath);

inline std::vector<std::byte> readBinaryFileBytes(const std::filesystem::path& path) {
  return readBinaryFile<std::byte>(path);
}

inline std::vector<u32> readBinaryFileU32(const std::filesystem::path& path) {
  return readBinaryFile<u32>(path);
}

} // namespace aur