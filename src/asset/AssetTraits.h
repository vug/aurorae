#pragma once

// #include "Material.h"
#include "Mesh.h"
#include "Shader.h"
#include "ShaderStage.h"

namespace aur {
// Trait to map definition types to asset types
template <typename TDefinition>
struct AssetTypeFor {
  // No default implementation - will cause compile error for unsupported types
};

// Specializations for supported mappings
template <>
struct AssetTypeFor<asset::ShaderStageDefinition> {
  using type = asset::ShaderStage;
};
template <>
struct AssetTypeFor<asset::ShaderDefinition> {
  using type = asset::Shader;
};
// template<>
// struct AssetTypeFor<asset::MaterialDefinition> {
//   using type = asset::Material;
// };
template <>
struct AssetTypeFor<asset::MeshDefinition> {
  using type = asset::Mesh;
};

// Helper alias for cleaner syntax
template <typename TDefinition>
using AssetTypeFor_t = typename AssetTypeFor<TDefinition>::type;

// Reverse mapping (asset type to definition type)
template <typename TAsset>
struct DefinitionTypeFor {};

template <>
struct DefinitionTypeFor<asset::ShaderStage> {
  using type = asset::ShaderStageDefinition;
};
template <>
struct DefinitionTypeFor<asset::Shader> {
  using type = asset::ShaderDefinition;
};
// template <>
// struct DefinitionTypeFor<asset::Material> {
//   using type = asset::MaterialDefinition;
// };
template <>
struct DefinitionTypeFor<asset::Mesh> {
  using type = asset::MeshDefinition;
};

// Cache type mapping
template <typename TDefinition>
struct CacheTypeFor {
  using type = std::unordered_map<StableId<TDefinition>, Handle<AssetTypeFor_t<TDefinition>>>;
};

template <typename TDefinition>
using CacheTypeFor_t = typename CacheTypeFor<TDefinition>::type;
} // namespace aur