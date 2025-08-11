#pragma once

#include "AssetRegistry.h"

#include <unordered_map>
#include <vector>

#include "../Handle.h"
#include "AssetTraits.h"
#include "GraphicsProgram.h"
#include "Material.h"
#include "Mesh.h"
#include "ShaderStage.h"

namespace aur {

namespace asset {
class Mesh;
} // namespace asset

class AssetManager {
public:
  explicit AssetManager(AssetRegistry& registry);

  template <AssetConcept TAsset>
  Handle<TAsset> load(const AssetUuid& uuid);
  template <AssetConcept TAsset>
  Handle<TAsset> load(const StableId<TAsset>& stableId);
  template <AssetConcept TAsset>
  Handle<TAsset> loadFromDefinition(typename TAsset::DefinitionType&& def);

  static constexpr size_t kMaxShaderStageCnt = 10'000;
  static constexpr size_t kMaxGraphicsProgramCnt = 5'000;
  static constexpr size_t kMaxMaterialCnt = 10'000;
  static constexpr size_t kMaxMeshCnt = 10'000;

  Handle<asset::Mesh> registerExistingMesh(asset::Mesh& mesh);

  [[nodiscard]] inline const asset::ShaderStage* get(Handle<asset::ShaderStage> handle) const {
    return &shaderStages_.at(handle);
  }
  [[nodiscard]] inline const asset::GraphicsProgram* get(Handle<asset::GraphicsProgram> handle) const {
    return &graphicsPrograms_.at(handle);
  }
  [[nodiscard]] inline const asset::Material* get(Handle<asset::Material> handle) const {
    return &materials_.at(handle);
  }
  [[nodiscard]] inline const asset::Mesh* get(Handle<asset::Mesh> handle) const {
    return &meshes_.at(handle);
  };

  void addGraphicsProgramUpdateListener(asset::GraphicsProgramUpdateCallback callback);

private:
  AssetRegistry* registry_{};

  // The manager OWNS the actual asset data in vectors.
  template <AssetConcept TAsset>
  typename TAsset::StorageType& getStorage();
  asset::ShaderStage::StorageType shaderStages_;
  asset::GraphicsProgram::StorageType graphicsPrograms_;
  asset::Material::StorageType materials_;
  asset::Mesh::StorageType meshes_;

  // Caching to prevent loading the same file twice
  template <AssetConcept TAsset>
  typename TAsset::CacheType& getCache();
  asset::ShaderStage::CacheType loadedShaderStages_;
  asset::GraphicsProgram::CacheType loadedGraphicsPrograms_;
  asset::Material::CacheType loadedMaterials_;
  asset::Mesh::CacheType loadedMeshes_;

  std::vector<asset::GraphicsProgramUpdateCallback> graphicsProgramUpdateListeners_;
  // std::vector<asset::ShaderDeleteCallback> shaderDeleteListeners_;
  void notifyGraphicsProgramUpdated(Handle<asset::GraphicsProgram> hnd) const;

  Handle<asset::ShaderStage> loadShaderStageFromDefinition(asset::ShaderStageDefinition&& shaderStageDef);
  Handle<asset::GraphicsProgram>
  loadGraphicsProgramFromDefinition(asset::GraphicsProgramDefinition&& graphicsProgramDef);
  Handle<asset::Material> loadMaterialFromDefinition(asset::MaterialDefinition&& materialDef);
  Handle<asset::Mesh> loadMeshFromDefinition(asset::MeshDefinition&& meshDef);
};

} // namespace aur