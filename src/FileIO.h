#pragma once
#include <filesystem>
#include <vector>

#include "Utils.h"

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

// Utility function to write a binary file
bool writeBinaryFile(const std::filesystem::path& filePath, const void* data, size_t sizeBytes);

// Convenience wrapper that writes a std::string to a binary file
inline bool writeBinaryFile(const std::filesystem::path& filePath, const std::string& data) {
  return writeBinaryFile(filePath, data.data(), data.size());
}

} // namespace aur