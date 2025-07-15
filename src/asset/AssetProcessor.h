#pragma once

#include <filesystem>

#include "Mesh.h"
#include "Shader.h"

namespace aur {

class AssetProcessor {
public:
  std::optional<asset::ShaderDefinition> processShader(const std::filesystem::path& shaderPath);

  std::optional<asset::ShaderDefinition> static loadShader(const std::filesystem::path& vertSpirvPath,
                                                           const std::filesystem::path& fragSpirvPath);

  std::vector<asset::MeshDefinition> static processMeshes(const std::filesystem::path& modelPath);

  static bool validateSpirV(const std::vector<std::byte>& blob);
};

} // namespace aur