#pragma once

#include <unordered_map>
#include <vector>

#include "../Handle.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"

#include <filesystem>

namespace aur {

namespace asset {
class Mesh;
} // namespace asset

class AssetManager {
public:
  Handle<asset::ShaderStage>
  loadShaderStageFromDefinition(const asset::ShaderStageDefinition& shaderStageDef);
  Handle<asset::Shader> loadShaderFromDefinition(const asset::ShaderDefinition& shaderDef);
  Handle<asset::Material> loadMaterialFromDefinition(const asset::MaterialDefinition& materialDef);
  Handle<asset::Mesh> loadMeshFromDefinition(const asset::MeshDefinition& meshDef);
  Handle<asset::Mesh> registerExistingMesh(asset::Mesh& mesh);

  [[nodiscard]] inline const asset::ShaderStage* get(Handle<asset::ShaderStage> handle) const {
    return &shaderStages_.at(handle);
  }
  [[nodiscard]] inline const asset::Shader* get(Handle<asset::Shader> handle) const {
    return &shaders_.at(handle);
  }
  [[nodiscard]] inline const asset::Material* get(Handle<asset::Material> handle) const {
    return &materials_.at(handle);
  }
  [[nodiscard]] inline const asset::Mesh* get(Handle<asset::Mesh> handle) const {
    return &meshes_.at(handle);
  };

  void addShaderUpdateListener(asset::ShaderUpdateCallback callback);

private:
  // The manager OWNS the actual asset data in vectors.
  std::vector<asset::ShaderStage> shaderStages_;
  std::vector<asset::Shader> shaders_;
  std::vector<asset::Material> materials_;
  std::vector<asset::Mesh> meshes_;

  // TODO(vug): implement caching, below members are not used at the moment
  // Caching to prevent loading the same file twice
  // std::unordered_map<std::string, Handle<asset::ShaderStage>> loadedShaderStages_;
  // std::unordered_map<std::string, Handle<asset::Shader>> loadedShaders_;
  // std::unordered_map<std::string, Handle<asset::Material>> loadedMaterials_;
  // std::unordered_map<std::filesystem::path, std::vector<Handle<asset::Mesh>>> loadedModels_;

  std::vector<asset::ShaderUpdateCallback> shaderUpdateListeners_;
  // std::vector<asset::ShaderDeleteCallback> shaderDeleteListeners_;
  void notifyShaderUpdated(Handle<asset::Shader> hnd) const;
};

} // namespace aur