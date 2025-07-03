#pragma once

#include "../Resources/DescriptorSet.h"
#include "../Resources/ShaderModule.h"
#include "Handle.h"

namespace aur::asset {

struct Shader;
struct Pipeline;

struct MaterialCreateInfo {
  // plan for asset::Material
  Handle<Shader> vertexHandle;
  Handle<Shader> fragHandle;

  // MaterialMetadata using which we can create the PipelineCreateInfo
  // Schema of material parameters, their types (options, ranges, texture, numbers, vec2s etc) and stored
  // values. Then, Renderer::getOrCreateMaterial() takes this create info and creates a renderer::Shader, a
  // Pipeline object, and corresponding buffers and images
};

struct Material {
  // asset::Material
  // will contain a reference to asset::Shader

  // render::Material
  Pipeline* pipeline;
  DescriptorSet descriptorSet;
};

} // namespace aur::asset
