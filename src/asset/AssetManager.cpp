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
HandleTypeFor_t<TDefinition> AssetManager::load(const StableId<TDefinition>& stableId) {
  CacheTypeFor_t<TDefinition>& assetCache = getCache<TDefinition>();
  auto defIt = assetCache.find(stableId);
  if (defIt != assetCache.end())
    return defIt->second;

  std::optional<TDefinition> defOpt = registry_->getDefinition(stableId);
  if (!defOpt.has_value()) {
    log().warn("Could not find definition for shader stage with id: {} in asset registry {}", stableId,
               registry_->getFilePath().generic_string());
    return {};
  }

  const HandleTypeFor_t<TDefinition> handle = loadFromDefinition(std::move(defOpt.value()));
  assetCache[stableId] = handle;

  return handle;
}

template <AssetDefinition TDefinition>
HandleTypeFor_t<TDefinition> AssetManager::loadFromDefinition(TDefinition&& def) {
  using TAsset = AssetTypeFor_t<TDefinition>;
  if constexpr (std::is_same_v<TAsset, asset::ShaderStage>) {
    return loadShaderStageFromDefinition(std::move(def));
  } else if constexpr (std::is_same_v<TAsset, asset::Shader>) {
    return loadShaderFromDefinition(std::move(def));
  } else if constexpr (std::is_same_v<TAsset, asset::Material>) {
    return loadMaterialFromDefinition(std::move(def));
  } else if constexpr (std::is_same_v<TAsset, asset::Mesh>) {
    return loadMeshFromDefinition(std::move(def));
  } else {
    static_assert("Unimplemented asset type");
    std::unreachable();
  }
}

template <AssetDefinition TDefinition>
StorageTypeFor_t<TDefinition>& AssetManager::getStorage() {
  using TAsset = AssetTypeFor_t<TDefinition>;
  if constexpr (std::is_same_v<TAsset, asset::ShaderStage>) {
    return shaderStages_;
  } else if constexpr (std::is_same_v<TAsset, asset::Shader>) {
    return shaders_;
  } else if constexpr (std::is_same_v<TAsset, asset::Material>) {
    return materials_;
  } else if constexpr (std::is_same_v<TAsset, asset::Mesh>) {
    return meshes_;
  } else {
    static_assert(AssetType<TAsset>, "Asset type doesn't support caching");
    std::unreachable();
  }
}

template <AssetDefinition TDefinition>
CacheTypeFor_t<TDefinition>& AssetManager::getCache() {
  using TAsset = AssetTypeFor_t<TDefinition>;
  if constexpr (std::is_same_v<TAsset, asset::ShaderStage>) {
    return loadedShaderStages_;
  } else if constexpr (std::is_same_v<TAsset, asset::Shader>) {
    return loadedShaders_;
  } else if constexpr (std::is_same_v<TAsset, asset::Material>) {
    return loadedMaterials_;
  } else if constexpr (std::is_same_v<TAsset, asset::Mesh>) {
    return loadedMeshes_;
  } else {
    static_assert(AssetType<TAsset>, "Asset type doesn't support caching");
    std::unreachable();
  }
}

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

#define EXPLICITLY_INSTANTIATE_TEMPLATES(DefinitionType)                                                     \
  template HandleTypeFor_t<DefinitionType> AssetManager::load<DefinitionType>(                               \
      const StableId<DefinitionType>& stableId);
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::ShaderDefinition)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::ShaderStageDefinition)
#undef EXPLICITLY_INSTANTIATE_TEMPLATES

#define EXPLICITLY_INSTANTIATE_TEMPLATES(DefinitionType)                                                     \
  template CacheTypeFor_t<DefinitionType>& AssetManager::getCache<DefinitionType>();                         \
  template HandleTypeFor_t<DefinitionType> AssetManager::loadFromDefinition<DefinitionType>(                 \
      DefinitionType && def);
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::ShaderDefinition)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::ShaderStageDefinition)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::MaterialDefinition)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::MeshDefinition)
#undef EXPLICITLY_INSTANTIATE_TEMPLATES

} // namespace aur