#pragma once

#include "../Handle.h"
#include "../VulkanWrappers.h"
#include "AssetTraits.h"

namespace aur {
// All possible leaf types
using ShaderValue = std::variant<i32, u32, f32, glm::vec2, glm::vec3, glm::vec4, glm::mat3, glm::mat4>;
} // namespace aur
namespace glz {
template <>
struct meta<glm::vec2> {
  using T = glm::vec2;
  static constexpr auto value = glz::array(&T::x, &T::y);
};
template <>
struct meta<glm::vec3> {
  static constexpr auto value = glz::array(3);
};
template <>
struct meta<glm::vec4> {
  static constexpr auto value = glz::array(4);
};
template <>
struct meta<glm::mat3> {
  static constexpr auto value = glz::array(9);
};
template <>
struct meta<glm::mat4> {
  // x1, x2, y1, y2...
  static constexpr auto value = glz::array(16);
};

template <>
struct meta<aur::ShaderValue> {
  static constexpr std::string_view tag = "shaderType"; // The JSON field to use as a tag
  static constexpr auto ids =
      std::array{"int",  "uint", "float", "vec2",
                 "vec3", "vec4", "mat3",  "mat4"}; // The IDs corresponding to the variant types
};

} // namespace glz

namespace aur {
enum class BlendingPreset {
  NoBlend,    // Opaque
  AlphaBlend, // Transparent
  Additive,
  // AlphaToCoverage,
  // PremultipliedAlpha,
  // Overlay,
  // Subtractive,
  // Xor, // Highlight
  // Invert,
  // And, // Mask
};
} // namespace aur
template <>
struct glz::meta<aur::BlendingPreset> {
  using enum aur::BlendingPreset;
  static constexpr auto value = glz::enumerate(NoBlend, AlphaBlend, Additive);
};

namespace aur::asset {
class GraphicsProgram;
struct Pipeline;

struct MaterialDefinition {
  // Increment version after each change to the schema or processing logic
  static constexpr u32 schemaVersion{1};
  u32 version{schemaVersion};

  AssetRef graphicsProgram;
  bool depthTest{true};
  bool depthWrite{true};
  PolygonMode polygonMode{PolygonMode::Fill};
  CullMode cullMode{CullMode::Back};
  FrontFace frontFace{FrontFace::CounterClockwise};
  f32 lineWidth{1.0f};
  BlendingPreset blendPreset{BlendingPreset::NoBlend};
  ShaderValue testShaderValue;
  glm::mat3 foo{1, 2, 3, 4, 5, 6, 7, 8, 9};
  glm::mat4 bar{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

  // MaterialMetadata using which we can create the PipelineCreateInfo
  // Schema of material parameters, their types (options, ranges, texture, numbers, vec2s etc) and stored
  // values. Then, Renderer::getOrCreateMaterial() takes this create info and creates a
  // renderer::GraphicsProgram, a Pipeline object, and corresponding buffers and images
};

class Material : public AssetTypeMixin<Material, MaterialDefinition, AssetType::Material, "Material",
                                       "019870da-2c87-7f9e-aece-9484ce47cac9"> {
public:
  static Material create(MaterialDefinition&& matDef, Handle<GraphicsProgram> graphProg);

  ~Material() = default;
  Material(const Material&) = delete;
  Material& operator=(const Material&) = delete;
  Material(Material&& other) noexcept = default;
  Material& operator=(Material&& other) noexcept = default;

  [[nodiscard]] inline const Handle<GraphicsProgram>& getGraphicsProgramHandle() const {
    return graphicsProgram_;
  }
  [[nodiscard]] inline const MaterialDefinition& getDefinition() const { return materialDef_; }

private:
  Material() = default;
  std::string debugName_;

  Handle<GraphicsProgram> graphicsProgram_;
  MaterialDefinition materialDef_;
};

} // namespace aur::asset
