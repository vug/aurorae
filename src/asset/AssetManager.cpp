#include "AssetManager.h"

#include <vector>

#include "../AppContext.h"
#include "AssetTraits.h"
#include "Mesh.h"
#include "Shader.h"

namespace aur {

AssetManager::AssetManager(AssetRegistry& registry)
    : registry_{&registry} {}

template <AssetDefinition TDefinition>
Handle<AssetTypeFor_t<TDefinition>> AssetManager::load(const StableId<TDefinition>& stableId) {
  using TAsset = AssetTypeFor_t<TDefinition>;

  CacheTypeFor_t<TDefinition>* assetCache;
  if constexpr (std::is_same_v<TAsset, asset::ShaderStage>) {
    assetCache = &loadedShaderStages_;
  } else if constexpr (std::is_same_v<TAsset, asset::Shader>) {
    assetCache = &loadedShaders_;
  } else {
    static_assert("Unimplemented asset type");
  }

  assert(assetCache);
  auto defIt = assetCache->find(stableId);
  if (defIt != assetCache->end())
    return defIt->second;

  std::optional<TDefinition> defOpt = registry_->getDefinition(stableId);
  if (!defOpt.has_value()) {
    log().warn("Could not find definition for shader stage with id: {} in asset registry {}", stableId,
               registry_->getFilePath().generic_string());
    return {};
  }

  const Handle<TAsset> handle = [this, &defOpt, &stableId]() {
    if constexpr (std::is_same_v<TAsset, asset::ShaderStage>) {
      const auto handle = loadShaderStageFromDefinition(std::move(defOpt.value()));
      loadedShaderStages_[stableId] = handle;
      return handle;
    } else if constexpr (std::is_same_v<TAsset, asset::Shader>) {
      const auto handle = loadShaderFromDefinition(std::move(defOpt.value()));
      loadedShaders_[stableId] = handle;
      return handle;
    } else {
      static_assert("Unimplemented asset type");
    }
  }();

  return handle;
}

template Handle<AssetTypeFor_t<asset::ShaderStageDefinition>>
AssetManager::load(const StableId<asset::ShaderStageDefinition>& stableId);

template Handle<AssetTypeFor_t<asset::ShaderDefinition>>
AssetManager::load(const StableId<asset::ShaderDefinition>& stableId);

Handle<asset::ShaderStage>
AssetManager::loadShaderStageFromDefinition(asset::ShaderStageDefinition&& shaderStageDef) {
  asset::ShaderStage shaderStage = asset::ShaderStage::create(std::move(shaderStageDef));
  shaderStages_.push_back(std::move(shaderStage));
  return Handle<asset::ShaderStage>{static_cast<u32>(shaderStages_.size() - 1)};
}

Handle<asset::Shader> AssetManager::loadShaderFromDefinition(const asset::ShaderDefinition& shaderDef) {
  Handle<asset::ShaderStage> vert = load(shaderDef.vert);
  if (!vert.isValid())
    return {};
  Handle<asset::ShaderStage> frag = load(shaderDef.frag);
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