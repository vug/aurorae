#include "AssetManager.h"

#include <vector>

#include "../AppContext.h"
#include "AssetProcessor.h"
#include "Mesh.h"
#include "Shader.h"

namespace aur {

AssetManager::AssetManager(AssetRegistry& registry)
    : registry_{&registry} {}

Handle<asset::ShaderStage>
AssetManager::loadShaderStage(const StableId<asset::ShaderStageDefinition>& stableId) {
  auto defIt = loadedShaderStages_.find(stableId);
  if (defIt != loadedShaderStages_.end())
    return defIt->second;

  std::optional<asset::ShaderStageDefinition> defOpt = registry_->getDefinition(stableId);
  if (!defOpt.has_value()) {
    log().warn("Could not find definition for shader stage with id: {} in asset registry {}", stableId,
               registry_->getFilePath().generic_string());
    return {};
  }

  const auto handle = loadShaderStageFromDefinition(std::move(defOpt.value()));
  loadedShaderStages_[stableId] = handle;

  return handle;
}

Handle<asset::ShaderStage>
AssetManager::loadShaderStageFromDefinition(asset::ShaderStageDefinition&& shaderStageDef) {
  asset::ShaderStage shaderStage = asset::ShaderStage::create(std::move(shaderStageDef));
  shaderStages_.push_back(std::move(shaderStage));
  return Handle<asset::ShaderStage>{static_cast<u32>(shaderStages_.size() - 1)};
}

Handle<asset::Shader> AssetManager::loadShaderFromDefinition(const asset::ShaderDefinition& shaderDef) {
  Handle<asset::ShaderStage> vert = loadShaderStage(shaderDef.vert);
  if (!vert.isValid())
    return {};
  Handle<asset::ShaderStage> frag = loadShaderStage(shaderDef.frag);
  if (!frag.isValid())
    return {};
  asset::Shader shader = asset::Shader::create(shaderDef, vert, frag);
  shaders_.push_back(std::move(shader));
  return Handle<asset::Shader>{static_cast<u32>(shaders_.size() - 1)};
}
Handle<asset::Material>
AssetManager::loadMaterialFromDefinition(const asset::MaterialDefinition& materialDef) {
  asset::Material material = asset::Material::create(materialDef);
  materials_.push_back(std::move(material));
  return Handle<asset::Material>{static_cast<u32>(materials_.size() - 1)};
}

Handle<asset::Mesh> AssetManager::loadMeshFromDefinition(const asset::MeshDefinition& meshDef) {
  asset::Mesh mesh = asset::Mesh::create(meshDef);
  meshes_.push_back(std::move(mesh));
  return Handle<asset::Mesh>{static_cast<u32>(meshes_.size() - 1)};
}

Handle<asset::Mesh> AssetManager::registerExistingMesh(asset::Mesh& mesh) {
  meshes_.push_back(std::move(mesh));
  return Handle<asset::Mesh>{static_cast<u32>(meshes_.size() - 1)};
}

void AssetManager::addShaderUpdateListener(asset::ShaderUpdateCallback callback) {
  shaderUpdateListeners_.push_back(std::move(callback));
}
void AssetManager::notifyShaderUpdated(Handle<asset::Shader> hnd) const {
  for (const auto& callback : shaderUpdateListeners_)
    callback(hnd);
}
} // namespace aur