#include "AssetManager.h"

#include <vector>

#include "../AppContext.h"
#include "AssetTraits.h"
#include "GraphicsProgram.h"
#include "Mesh.h"

namespace aur {

AssetManager::AssetManager(AssetRegistry& registry)
    : registry_{&registry} {}

template <AssetConcept TAsset>
Handle<TAsset> AssetManager::load(const AssetUuid& uuid) {
  typename TAsset::CacheType& assetCache = getCache<TAsset>();
  auto defIt = assetCache.find(uuid);
  if (defIt != assetCache.end())
    return defIt->second;

  using TDefinition = typename TAsset::DefinitionType;
  std::optional<TDefinition> defOpt = registry_->getDefinition<TDefinition>(uuid);
  if (!defOpt.has_value())
    return {};

  const Handle<TAsset> handle = loadFromDefinition<TAsset>(std::move(defOpt.value()));
  assetCache[uuid] = handle;

  return handle;
}

template <AssetConcept TAsset>
Handle<TAsset> AssetManager::load(const StableId<TAsset>& stableId) {

  std::optional<AssetUuid> uuidOpt = registry_->getUuid(stableId);
  if (!uuidOpt) {
    log().warn("Could not find definition for asset with stable id: {} in asset registry {}", stableId,
               registry_->getPath().generic_string());
    return {};
  }
  return load<TAsset>(*uuidOpt);
}

template <AssetConcept TAsset>
Handle<TAsset> AssetManager::loadFromDefinition(typename TAsset::DefinitionType&& def) {
  if constexpr (std::is_same_v<TAsset, asset::ShaderStage>) {
    return loadShaderStageFromDefinition(std::move(def));
  } else if constexpr (std::is_same_v<TAsset, asset::GraphicsProgram>) {
    return loadGraphicsProgramFromDefinition(std::move(def));
  } else if constexpr (std::is_same_v<TAsset, asset::Material>) {
    return loadMaterialFromDefinition(std::move(def));
  } else if constexpr (std::is_same_v<TAsset, asset::Mesh>) {
    return loadMeshFromDefinition(std::move(def));
  } else {
    static_assert("Unimplemented asset type");
    std::unreachable();
  }
}

template <AssetConcept TAsset>
typename TAsset::StorageType& AssetManager::getStorage() {
  if constexpr (std::is_same_v<TAsset, asset::ShaderStage>) {
    return shaderStages_;
  } else if constexpr (std::is_same_v<TAsset, asset::GraphicsProgram>) {
    return graphicsPrograms_;
  } else if constexpr (std::is_same_v<TAsset, asset::Material>) {
    return materials_;
  } else if constexpr (std::is_same_v<TAsset, asset::Mesh>) {
    return meshes_;
  } else {
    static_assert(AssetConcept<TAsset>, "Asset type doesn't support caching");
    std::unreachable();
  }
}

template <AssetConcept TAsset>
typename TAsset::CacheType& AssetManager::getCache() {
  if constexpr (std::is_same_v<TAsset, asset::ShaderStage>) {
    return loadedShaderStages_;
  } else if constexpr (std::is_same_v<TAsset, asset::GraphicsProgram>) {
    return loadedGraphicsPrograms_;
  } else if constexpr (std::is_same_v<TAsset, asset::Material>) {
    return loadedMaterials_;
  } else if constexpr (std::is_same_v<TAsset, asset::Mesh>) {
    return loadedMeshes_;
  } else {
    static_assert(AssetConcept<TAsset>, "Asset type doesn't support caching");
    std::unreachable();
  }
}

Handle<asset::ShaderStage>
AssetManager::loadShaderStageFromDefinition(asset::ShaderStageDefinition&& shaderStageDef) {
  asset::ShaderStage shaderStage = asset::ShaderStage::create(std::move(shaderStageDef));
  shaderStages_.push_back(std::move(shaderStage));
  return Handle<asset::ShaderStage>{static_cast<u32>(shaderStages_.size() - 1)};
}

Handle<asset::GraphicsProgram>
AssetManager::loadGraphicsProgramFromDefinition(asset::GraphicsProgramDefinition&& graphicsProgramDef) {
  const Handle vert{load<asset::ShaderStage>(graphicsProgramDef.vert.getUuid())};
  if (!vert.isValid())
    return {};
  const Handle frag{load<asset::ShaderStage>(graphicsProgramDef.frag.getUuid())};
  if (!frag.isValid())
    return {};
  asset::GraphicsProgram graphicsProgram = asset::GraphicsProgram::create(vert, frag);
  graphicsPrograms_.push_back(std::move(graphicsProgram));
  return Handle<asset::GraphicsProgram>{static_cast<u32>(graphicsPrograms_.size() - 1)};
}
Handle<asset::Material> AssetManager::loadMaterialFromDefinition(asset::MaterialDefinition&& materialDef) {
  const Handle graphProg{load<asset::GraphicsProgram>(materialDef.graphicsProgram.getUuid())};
  asset::Material material = asset::Material::create(graphProg);
  materials_.push_back(std::move(material));
  return Handle<asset::Material>{static_cast<u32>(materials_.size() - 1)};
}

Handle<asset::Mesh> AssetManager::loadMeshFromDefinition(asset::MeshDefinition&& meshDef) {
  asset::Mesh mesh = asset::Mesh::create(std::move(meshDef));
  meshes_.push_back(std::move(mesh));
  return Handle<asset::Mesh>{static_cast<u32>(meshes_.size() - 1)};
}

Handle<asset::Mesh> AssetManager::registerExistingMesh(asset::Mesh& mesh) {
  meshes_.push_back(std::move(mesh));
  return Handle<asset::Mesh>{static_cast<u32>(meshes_.size() - 1)};
}

void AssetManager::addGraphicsProgramUpdateListener(asset::GraphicsProgramUpdateCallback callback) {
  graphicsProgramUpdateListeners_.push_back(std::move(callback));
}
void AssetManager::notifyGraphicsProgramUpdated(Handle<asset::GraphicsProgram> hnd) const {
  for (const auto& callback : graphicsProgramUpdateListeners_)
    callback(hnd);
}

#define EXPLICITLY_INSTANTIATE_TEMPLATES(TAsset)                                                             \
  template Handle<TAsset> AssetManager::load<TAsset>(const AssetUuid& uuid);                                 \
  template Handle<TAsset> AssetManager::load<TAsset>(const StableId<TAsset>& stableId);
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::GraphicsProgram)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::ShaderStage)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::Material)
#undef EXPLICITLY_INSTANTIATE_TEMPLATES

#define EXPLICITLY_INSTANTIATE_TEMPLATES(TAsset)                                                             \
  template typename TAsset::CacheType& AssetManager::getCache<TAsset>();                                     \
  template Handle<TAsset> AssetManager::loadFromDefinition<TAsset>(typename TAsset::DefinitionType && def);
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::GraphicsProgram)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::ShaderStage)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::Material)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::Mesh)
#undef EXPLICITLY_INSTANTIATE_TEMPLATES

} // namespace aur