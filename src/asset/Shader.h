#pragma once

#include <filesystem>

namespace aur::asset {

struct Shader {
  std::filesystem::path vertPath;
  std::filesystem::path fragPath;
};

} // namespace aur::asset
