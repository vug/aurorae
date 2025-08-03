#pragma once

#include <filesystem>
#include <memory>

#include "AssetRegistry.h"
#include "GraphicsProgram.h"
#include "Mesh.h"
#include "ShaderStage.h"

namespace shaderc {
class Compiler;
}

namespace aur {

class AssetProcessor {
public:
  AssetProcessor(AssetRegistry& registry);
  ~AssetProcessor();

  DefinitionType extensionToDefinitionType(std::filesystem::path ext);

  void processAllAssets();
  void processAsset(const std::filesystem::path& assetPath);
  void processOnlyNeedingAssets();

  enum class ShaderBuildMode {
    Debug,
    Release,
  };
  std::optional<asset::ShaderStageDefinition> processShaderStage(const std::filesystem::path& srcPath,
                                                                 ShaderBuildMode buildMode);
  std::optional<asset::GraphicsProgramDefinition>
  processGraphicsProgram(const std::filesystem::path& srcPath);
  std::vector<asset::MeshDefinition> static processMeshes(const std::filesystem::path& modelPath);

  template <AssetDefinitionConcept TDefinition>
  static AssetUuid makeUuid(const StableId<TDefinition>& stableId);

  static bool validateSpirV(const std::vector<u32>& blob);

private:
  AssetRegistry* registry_;
  std::unique_ptr<shaderc::Compiler> shaderCompiler_;
};
} // namespace aur