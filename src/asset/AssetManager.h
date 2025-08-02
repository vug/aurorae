#pragma once

#include "AssetRegistry.h"

#include <unordered_map>
#include <vector>

#include "../Handle.h"
#include "AssetTraits.h"
#include "Common.h"
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
  AssetManager(AssetRegistry& registry);

  template <AssetDefinition TDefinition>
  HandleTypeFor_t<TDefinition> load(const StableId<TDefinition>& stableId);
  template <AssetDefinition TDefinition>
  HandleTypeFor_t<TDefinition> loadFromDefinition(TDefinition&& def);

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
  template <AssetDefinition TDefinition>
  StorageTypeFor_t<TDefinition>& getStorage();
  StorageTypeFor_t<asset::ShaderStageDefinition> shaderStages_;
  StorageTypeFor_t<asset::GraphicsProgramDefinition> graphicsPrograms_;
  StorageTypeFor_t<asset::MaterialDefinition> materials_;
  StorageTypeFor_t<asset::MeshDefinition> meshes_;

  // Caching to prevent loading the same file twice
  template <AssetDefinition TDefinition>
  CacheTypeFor_t<TDefinition>& getCache();
  CacheTypeFor_t<asset::ShaderStageDefinition> loadedShaderStages_;
  CacheTypeFor_t<asset::GraphicsProgramDefinition> loadedGraphicsPrograms_;
  CacheTypeFor_t<asset::MaterialDefinition> loadedMaterials_;
  CacheTypeFor_t<asset::MeshDefinition> loadedMeshes_;

  std::vector<asset::GraphicsProgramUpdateCallback> graphicsProgramUpdateListeners_;
  // std::vector<asset::ShaderDeleteCallback> shaderDeleteListeners_;
  void notifyGraphicsProgramUpdated(Handle<asset::GraphicsProgram> hnd) const;

  Handle<asset::ShaderStage> loadShaderStageFromDefinition(asset::ShaderStageDefinition&& shaderStageDef);
  Handle<asset::GraphicsProgram> loadGraphicsProgramFromDefinition(const asset::GraphicsProgramDefinition& graphicsProgramDef);
  Handle<asset::Material> loadMaterialFromDefinition(const asset::MaterialDefinition& materialDef);
  Handle<asset::Mesh> loadMeshFromDefinition(const asset::MeshDefinition& meshDef);
};

} // namespace aur