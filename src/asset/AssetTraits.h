#pragma once

#include "../Handle.h"

namespace aur {

// To prevent circular dependencies we forward declare all asset types here first
namespace asset {
struct ShaderStageDefinition;
struct GraphicsProgramDefinition;
struct MaterialDefinition;
struct MeshDefinition;

class ShaderStage;
class GraphicsProgram;
class Material;
class Mesh;
} // namespace asset

// Concepts
template <typename TDefinition>
concept AssetDefinition = std::is_same_v<TDefinition, asset::ShaderStageDefinition> ||
                          std::is_same_v<TDefinition, asset::GraphicsProgramDefinition> ||
                          std::is_same_v<TDefinition, asset::MaterialDefinition> ||
                          std::is_same_v<TDefinition, asset::MeshDefinition>;

template <typename TAsset>
concept AssetType =
    std::is_same_v<TAsset, asset::ShaderStage> || std::is_same_v<TAsset, asset::GraphicsProgram> ||
    std::is_same_v<TAsset, asset::Material> || std::is_same_v<TAsset, asset::Mesh>;

template <AssetType TAsset>
struct AssetWithCacheType {
  using CacheType = std::unordered_map<StableId<TAsset>, Handle<TAsset>>;
};

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
template <AssetType TAsset>
using DefinitionTypeFor_t = typename DefinitionTypeFor<TAsset>::type;

template <AssetType TAsset>
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
template <AssetDefinition TDefinition>
constexpr const char* AssetLabelFor_v = AssetLabelFor<TDefinition>::value;

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