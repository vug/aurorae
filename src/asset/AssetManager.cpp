#include "AssetManager.h"

#include <vector>

#include "../Logger.h"
#include "Mesh.h"
#include "Shader.h"

namespace aur {

Handle<asset::Shader> AssetManager::loadShaderFromDefinition(const asset::ShaderDefinition& shaderDef) {

  asset::Shader shader = asset::Shader::create(shaderDef);
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
} // namespace aur