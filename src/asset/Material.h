#pragma once

#include "../Handle.h"
#include "../VulkanWrappers.h"
#include "AssetTraits.h"

namespace aur {
enum class BlendingPreset {
  // Opaque
  NoBlend,
  // Transparent
  AlphaBlend,
  // Additive,
  // PremultipliedAlpha,
  // Multiply,
  // Screen,
  // AlphaToCoverage,
  //
  // // Highlight
  // Xor,
  // Invert,
  // // Mask
  // And,
  //
  // Subtractive,
  // Overlay,
};
}
template <>
struct glz::meta<aur::BlendingPreset> {
  using enum aur::BlendingPreset;
  static constexpr auto value = glz::enumerate(NoBlend, AlphaBlend);
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
