#pragma once

namespace aur {

enum class DefinitionType : unsigned int {
  ShaderStage = 0,
  GraphicsProgram = 1,
  Material = 2,
  Mesh = 3,
};

namespace asset {

// To prevent circular dependencies, we forward declare all asset types here first
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
concept AssetDefinitionConcept = std::is_same_v<TDefinition, asset::ShaderStageDefinition> ||
                                 std::is_same_v<TDefinition, asset::GraphicsProgramDefinition> ||
                                 std::is_same_v<TDefinition, asset::MaterialDefinition> ||
                                 std::is_same_v<TDefinition, asset::MeshDefinition>;

template <typename TAsset>
concept AssetConcept =
    std::is_same_v<TAsset, asset::ShaderStage> || std::is_same_v<TAsset, asset::GraphicsProgram> ||
    std::is_same_v<TAsset, asset::Material> || std::is_same_v<TAsset, asset::Mesh>;

constexpr std::array kAssetOrder = {DefinitionType::ShaderStage, DefinitionType::GraphicsProgram,
                                    DefinitionType::Material, DefinitionType::Mesh};
} // namespace aur

namespace glz {
template <>
struct meta<aur::DefinitionType> {
  using enum aur::DefinitionType;
  static constexpr auto value = glz::enumerate(ShaderStage, GraphicsProgram, Material, Mesh);
};
} // namespace glz