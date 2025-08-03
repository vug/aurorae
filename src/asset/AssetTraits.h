#pragma once

#include "../Handle.h"
#include "AssetConcepts.h"

namespace aur {

template <AssetConcept TAsset>
struct AssetWithCacheType {
  using CacheType = std::unordered_map<StableId<TAsset>, Handle<TAsset>>;
};

// Trait to map definition types to asset types
template <AssetDefinitionConcept TDefinition>
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
template <AssetDefinitionConcept TDefinition>
using AssetTypeFor_t = typename AssetTypeFor<TDefinition>::type;

// Reverse mapping (asset type to definition type)
template <AssetConcept TAsset>
struct DefinitionTypeFor {
  // no default implementation
};
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
template <AssetConcept TAsset>
using DefinitionTypeFor_t = typename DefinitionTypeFor<TAsset>::type;

template <AssetConcept TAsset>
struct AssetLabelFor {
  // no default implementation
};
template <>
struct AssetLabelFor<asset::ShaderStage> {
  static constexpr const char* value = "ShaderStage";
};
template <>
struct AssetLabelFor<asset::GraphicsProgram> {
  static constexpr const char* value = "GraphicsProgram";
};
template <>
struct AssetLabelFor<asset::Material> {
  static constexpr const char* value = "Material";
};
template <>
struct AssetLabelFor<asset::Mesh> {
  static constexpr const char* value = "Mesh";
};
template <AssetDefinitionConcept TDefinition>
constexpr const char* AssetLabelFor_v = AssetLabelFor<TDefinition>::value;

template <AssetDefinitionConcept TDefinition>
struct CacheTypeFor {
  using type = std::unordered_map<AssetUuid, Handle<AssetTypeFor_t<TDefinition>>>;
};
template <AssetDefinitionConcept TDefinition>
using CacheTypeFor_t = typename CacheTypeFor<TDefinition>::type;

template <AssetDefinitionConcept TDefinition>
struct StorageTypeFor {
  using type = std::vector<AssetTypeFor_t<TDefinition>>;
};
template <AssetDefinitionConcept TDefinition>
using StorageTypeFor_t = typename StorageTypeFor<TDefinition>::type;

template <AssetDefinitionConcept TDefinition>
struct HandleTypeFor {
  using type = Handle<AssetTypeFor_t<TDefinition>>;
};
template <AssetDefinitionConcept TDefinition>
using HandleTypeFor_t = typename HandleTypeFor<TDefinition>::type;

} // namespace aur