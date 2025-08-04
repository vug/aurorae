#pragma once

#include <filesystem>
#include <memory>

#include "AssetRegistry.h"

namespace shaderc {
class Compiler;
}

namespace aur {

class AssetProcessor {
public:
  explicit AssetProcessor(AssetRegistry& registry);
  ~AssetProcessor();

  AssetProcessor(const AssetProcessor& other) = delete;
  AssetProcessor(AssetProcessor&& other) noexcept = delete;
  AssetProcessor& operator=(const AssetProcessor& other) = delete;
  AssetProcessor& operator=(AssetProcessor&& other) noexcept = delete;

  static AssetType extensionToDefinitionType(const std::filesystem::path& ext);

  void processAllAssets();
  std::optional<AssetEntry> processAssetMakeEntry(const std::filesystem::path& assetPath);
  void processOnlyNeedingAssets();

  enum class ShaderBuildMode {
    Debug,
    Release,
  };
  [[nodiscard]] std::optional<asset::ShaderStageDefinition>
  processShaderStage(const std::filesystem::path& srcPath, ShaderBuildMode buildMode) const;
  std::optional<asset::GraphicsProgramDefinition> static processGraphicsProgram(
      const std::filesystem::path& srcPath);
  std::optional<asset::MaterialDefinition> static processMaterial(const std::filesystem::path& srcPath);
  std::optional<asset::MeshDefinition> static processMeshes(const std::filesystem::path& modelPath);

  template <AssetConcept TAsset>
  static AssetUuid makeUuid(const StableId<TAsset>& stableId);

  static bool validateSpirV(const std::vector<u32>& blob);

private:
  AssetRegistry* registry_;
  std::unique_ptr<shaderc::Compiler> shaderCompiler_;
};
} // namespace aur