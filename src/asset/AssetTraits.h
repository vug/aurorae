#pragma once

#include <modern-uuid/uuid.h>

#include "../Handle.h"
#include "AssetConcepts.h"
#include "AssetIds.h"

namespace aur {

// Mixin for asset types via CRTP. All asset classes need to publicly inherit from this.

template <typename TAsset, AssetDefinitionConcept TDefinition, AssetType TypeEnum, fixed_string Label,
          fixed_string<37> UuidNamespace>
struct AssetTypeMixin {
  using DefinitionType = TDefinition;
  static constexpr std::string_view label{Label};
  static constexpr muuid::uuid uuidNamespace{UuidNamespace.data};
  static constexpr AssetType typeEnum{TypeEnum};
  using CacheType = std::unordered_map<AssetUuid, Handle<TAsset>>;
  using StorageType = std::vector<TAsset>;
};

// --------------------

// We'll keep definitions pure structs w/o inheritance. Hence, their traits are declared via classic
// mechanisms.

// No default implementation - will cause compilation error for unsupported types
template <AssetDefinitionConcept TDefinition>
struct AssetTypeFor {};
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
} // namespace aur