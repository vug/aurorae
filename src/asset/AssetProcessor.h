#pragma once

#include <filesystem>

#include "Mesh.h"
#include "Shader.h"

namespace aur {

class AssetProcessor {
public:
  static std::optional<asset::ShaderStageDefinition> processShaderStage(const std::filesystem::path& srcPath);

  std::optional<asset::ShaderDefinition> static loadShader(const std::filesystem::path& vertSpirvPath,
                                                           const std::filesystem::path& fragSpirvPath);

  std::vector<asset::MeshDefinition> static processMeshes(const std::filesystem::path& modelPath);

  static bool validateSpirV(const std::vector<u32>& blob);
};

} // namespace aur