#pragma once

#include "../Handle.h"
#include "../Resources/DescriptorSet.h"

namespace aur::asset {

class Shader;
struct Pipeline;

struct MaterialDefinition {
  Handle<Shader> shaderHandle;

  // MaterialMetadata using which we can create the PipelineCreateInfo
  // Schema of material parameters, their types (options, ranges, texture, numbers, vec2s etc) and stored
  // values. Then, Renderer::getOrCreateMaterial() takes this create info and creates a renderer::Shader, a
  // Pipeline object, and corresponding buffers and images
};

class Material {
public:
  static Material create(const MaterialDefinition& createInfo);

  ~Material() = default;
  Material(const Material&) = delete;
  Material& operator=(const Material&) = delete;
  Material(Material&& other) noexcept = default;
  Material& operator=(Material&& other) noexcept = default;

  [[nodiscard]] inline Handle<Shader> getShaderHandle() const { return def_.shaderHandle; }

private:
  Material() = default;

  MaterialDefinition def_;

  // TO render::Material
  Pipeline* pipeline{};
  DescriptorSet descriptorSet;
};

} // namespace aur::asset
