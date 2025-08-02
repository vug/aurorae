#pragma once

#include <filesystem>

#include "AssetRegistry.h"
#include "GraphicsProgram.h"
#include "Mesh.h"
#include "ShaderStage.h"

namespace aur {

class AssetProcessor {
public:
  AssetProcessor(AssetRegistry& registry);

  DefinitionType extensionToDefinitionType(std::filesystem::path ext);

  void processAllAssets();
  void processOnlyNeedingAssets();

  enum class ShaderBuildMode {
    Debug,
    Release,
  };
  static std::optional<asset::ShaderStageDefinition> processShaderStage(const std::filesystem::path& srcPath,
                                                                        ShaderBuildMode buildMode);

  static std::optional<asset::GraphicsProgramDefinition> processGraphicsProgram(const std::filesystem::path& srcPath);

  std::vector<asset::MeshDefinition> static processMeshes(const std::filesystem::path& modelPath);

  static bool validateSpirV(const std::vector<u32>& blob);

private:
  AssetRegistry* registry_;
};

} // namespace aur