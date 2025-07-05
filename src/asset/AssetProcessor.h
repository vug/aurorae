#pragma once

#include <filesystem>

#include "Mesh.h"
#include "Shader.h"

namespace aur {

class AssetProcessor {
public:
  std::optional<asset::ShaderDefinition> static processShader(const std::filesystem::path& vertPath,
                                                              const std::filesystem::path& fragPath);

  std::vector<asset::MeshDefinition> static processMeshes(const std::filesystem::path& modelPath);

  static bool validateSpirV(const std::vector<std::byte>& blob);
};

} // namespace aur