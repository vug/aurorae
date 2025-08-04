#pragma once

#include "../Handle.h"
#include "../Resources/DescriptorSet.h"
#include "AssetTraits.h"

namespace aur::asset {

class GraphicsProgram;
struct Pipeline;

struct MaterialDefinition {
  Handle<GraphicsProgram> graphicsProgramHandle;

  // MaterialMetadata using which we can create the PipelineCreateInfo
  // Schema of material parameters, their types (options, ranges, texture, numbers, vec2s etc) and stored
  // values. Then, Renderer::getOrCreateMaterial() takes this create info and creates a
  // renderer::GraphicsProgram, a Pipeline object, and corresponding buffers and images
};

class Material : public AssetTypeMixin<Material, MaterialDefinition, AssetType::Material, "Material",
                                       "019870da-2c87-7f9e-aece-9484ce47cac9"> {
public:
  static Material create(MaterialDefinition&& createInfo);

  ~Material() = default;
  Material(const Material&) = delete;
  Material& operator=(const Material&) = delete;
  Material(Material&& other) noexcept = default;
  Material& operator=(Material&& other) noexcept = default;

  [[nodiscard]] inline Handle<GraphicsProgram> getGraphicsProgramHandle() const {
    return def_.graphicsProgramHandle;
  }

private:
  Material() = default;

  MaterialDefinition def_;
};

} // namespace aur::asset
