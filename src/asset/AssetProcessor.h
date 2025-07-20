#pragma once

#include <filesystem>

#include "Mesh.h"
#include "Shader.h"

namespace aur {

enum class DefinitionType : u32 {
  ShaderStage = 0,
  Shader = 1,
  Material = 2,
  Mesh = 3,
};

using AssetUuid = std::string;

struct AssetEntry {
  DefinitionType type;
  std::filesystem::path srcPath;
  std::filesystem::path dstPath;
  std::optional<std::string> subAssetName;
  std::optional<std::vector<AssetUuid>> dependencies;
};

struct AssetRegistry {
  std::unordered_map<AssetUuid, AssetEntry> entries;
  std::unordered_map<std::string, AssetUuid> aliases;
};

class AssetProcessor {
public:
  template <typename TDef>
  static std::optional<TDef> getDefinition(const std::filesystem::path& srcRelPath);

  static std::optional<asset::ShaderStageDefinition> processShaderStage(const std::filesystem::path& srcPath);

  std::optional<asset::ShaderDefinition> static loadShader(const std::filesystem::path& vertSpirvPath,
                                                           const std::filesystem::path& fragSpirvPath);

  std::vector<asset::MeshDefinition> static processMeshes(const std::filesystem::path& modelPath);

  static bool validateSpirV(const std::vector<u32>& blob);
};

} // namespace aur