#pragma once
#include <filesystem>
#include <vector>

namespace std {
enum class byte : unsigned char;
} // namespace std

namespace aur {
// Utility function to read a binary file
std::vector<std::byte> readBinaryFile(const std::filesystem::path& filePath);
} // namespace aur