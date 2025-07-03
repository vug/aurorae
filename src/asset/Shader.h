#pragma once

#include <filesystem>

#include "../FileIO.h"

namespace aur::asset {

struct Shader {
  std::filesystem::path vertPath;
  std::filesystem::path fragPath;
  BinaryBlob vertBlob;
  BinaryBlob fragBlob;

  std::string debugName;
};

} // namespace aur::asset
