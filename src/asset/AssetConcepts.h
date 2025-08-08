#pragma once

namespace aur {
enum class AssetType : unsigned int {
  ShaderStage = 0,
  GraphicsProgram = 1,
  Material = 2,
  Mesh = 3,
};
}
template <>
struct glz::meta<aur::AssetType> {
  using enum aur::AssetType;
  static constexpr auto value = glz::enumerate(ShaderStage, GraphicsProgram, Material, Mesh);
};

namespace aur {
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

template <size_t N>
struct fixed_string {
  char data[N];
  constexpr fixed_string(const char (&str)[N]) { std::copy_n(str, N, data); } // NOLINT: implicit
  constexpr operator std::string_view() const { return {data, N - 1}; }       // NOLINT: implicit
};

template <typename TAsset>
concept AssetConcept = requires {
  requires(std::is_same_v<TAsset, asset::ShaderStage> || std::is_same_v<TAsset, asset::GraphicsProgram> ||
           std::is_same_v<TAsset, asset::Material> || std::is_same_v<TAsset, asset::Mesh>);
};

constexpr std::array kAssetOrder = {AssetType::ShaderStage, AssetType::GraphicsProgram, AssetType::Material,
                                    AssetType::Mesh};
} // namespace aur
