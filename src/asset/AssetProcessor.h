#pragma once

#include <filesystem>

#include "Shader.h"

namespace aur {

class AssetProcessor {
public:
  asset::ShaderDefinition static processShader(const std::filesystem::path& vertPath,
                                               const std::filesystem::path& fragPath);

  // Validate a SPIR-V shader binary
  static bool validateSPIRV(const std::vector<std::byte>& blob);
};

} // namespace aur