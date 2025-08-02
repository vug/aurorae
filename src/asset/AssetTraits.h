#pragma once

#include "GraphicsProgram.h"
#include "Material.h"
#include "Mesh.h"
#include "ShaderStage.h"

namespace aur {

// Concepts
template <typename T>
concept AssetDefinition =
    std::is_same_v<T, asset::ShaderStageDefinition> || std::is_same_v<T, asset::GraphicsProgramDefinition> ||
    std::is_same_v<T, asset::MaterialDefinition> || std::is_same_v<T, asset::MeshDefinition>;

template <typename T>
concept AssetType = std::is_same_v<T, asset::GraphicsProgram> || std::is_same_v<T, asset::ShaderStage> ||
                    std::is_same_v<T, asset::Material> || std::is_same_v<T, asset::Mesh>;

// Trait to map definition types to asset types
template <AssetDefinition TDefinition>
struct AssetTypeFor {
  // No default implementation - will cause compile error for unsupported types
};

// Specializations for supported mappings
template <>
struct AssetTypeFor<asset::ShaderStageDefinition> {
  using type = asset::ShaderStage;
};
template <>
struct AssetTypeFor<asset::GraphicsProgramDefinition> {
  using type = asset::GraphicsProgram;
};
template <>
struct AssetTypeFor<asset::MaterialDefinition> {
  using type = asset::Material;
};
template <>
struct AssetTypeFor<asset::MeshDefinition> {
  using type = asset::Mesh;
};

// Helper alias for cleaner syntax
template <AssetDefinition TDefinition>
using AssetTypeFor_t = typename AssetTypeFor<TDefinition>::type;

// Reverse mapping (asset type to definition type)
template <AssetType TAsset>
struct DefinitionTypeFor {};

template <>
struct DefinitionTypeFor<asset::ShaderStage> {
  using type = asset::ShaderStageDefinition;
};
template <>
struct DefinitionTypeFor<asset::GraphicsProgram> {
  using type = asset::GraphicsProgramDefinition;
};
template <>
struct DefinitionTypeFor<asset::Material> {
  using type = asset::MaterialDefinition;
};
template <>
struct DefinitionTypeFor<asset::Mesh> {
  using type = asset::MeshDefinition;
};

template <AssetDefinition TDefinition>
struct CacheTypeFor {
  using type = std::unordered_map<StableId<TDefinition>, Handle<AssetTypeFor_t<TDefinition>>>;
};
template <AssetDefinition TDefinition>
using CacheTypeFor_t = typename CacheTypeFor<TDefinition>::type;

template <AssetDefinition TDefinition>
struct StorageTypeFor {
  using type = std::vector<AssetTypeFor_t<TDefinition>>;
};
template <AssetDefinition TDefinition>
using StorageTypeFor_t = typename StorageTypeFor<TDefinition>::type;

template <AssetDefinition TDefinition>
struct HandleTypeFor {
  using type = Handle<AssetTypeFor_t<TDefinition>>;
};
template <AssetDefinition TDefinition>
using HandleTypeFor_t = typename HandleTypeFor<TDefinition>::type;

} // namespace aur