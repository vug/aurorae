#include "AssetManager.h"

#include <vector>

#include "../AppContext.h"
#include "AssetProcessor.h"
#include "Mesh.h"
#include "Shader.h"

namespace aur {

Handle<asset::ShaderStage>
AssetManager::loadShaderStageFromDefinition(const asset::ShaderStageDefinition& shaderStageDef) {
  asset::ShaderStage shaderStage = asset::ShaderStage::create(shaderStageDef);
  shaderStages_.push_back(std::move(shaderStage));
  return Handle<asset::ShaderStage>{static_cast<u32>(shaderStages_.size() - 1)};
}

Handle<asset::Shader> AssetManager::loadShaderFromDefinition(const asset::ShaderDefinition& shaderDef) {
  // TODO(vug): make AssetManager refer to AssetRegistry instead of AssetRegistry
  auto ar = AppContext::get<AssetRegistry>();
  // TODO(vug): instead of giving ref to ShaderStageDefinition, make it an asset and give asset handle
  const asset::ShaderStageDefinition vertDef = ar.getDefinition(shaderDef.vert).value();
  const asset::ShaderStageDefinition fragDef = ar.getDefinition(shaderDef.frag).value();
  asset::Shader shader = asset::Shader::create(shaderDef, vertDef.spirv, fragDef.spirv);
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