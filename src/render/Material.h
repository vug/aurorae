#pragma once

#include "../Handle.h"
#include "../Pipeline.h"
#include "../Resources/Buffer.h"
#include "../Resources/DescriptorSet.h"
#include "../asset/GraphicsProgram.h"
#include "../asset/Material.h"
#include "../asset/ShaderReflection.h"

namespace aur {
class Renderer;
} // namespace aur

namespace aur::render {
class GraphicsProgram;

class Material {
public:
  Material() = default;
  Material(Renderer& renderer, Handle<asset::Material> asset);
  ~Material();

  Material(const Material& other) = delete;
  Material(Material&& other) noexcept = default;
  Material& operator=(const Material& other) = delete;
  Material& operator=(Material&& other) noexcept = default;

  void setParam(std::string_view name, std::span<const std::byte> data) const;

  [[nodiscard]] inline Handle<render::GraphicsProgram> getGraphicsProgramHandle() const {
    return graphicsProgramHandle_;
  }
  [[nodiscard]] inline const asset::ShaderStageSchema& getGraphicsProgramSchema() const {
    return assetHandle_->getGraphicsProgramHandle()->getCombinedSchema();
  }
  [[nodiscard]] inline const PipelineCreateInfo& getPipelineCreateInfo() const { return pipelineCreateInfo_; }
  [[nodiscard]] inline const Pipeline* getPipeline() const { return pipeline_; }

  [[nodiscard]] inline const DescriptorSet& getMatParamsDescriptorSet() const {
    return matParamsDescriptorSet_;
  }

private:
  Renderer* renderer_{};
  Handle<asset::Material> assetHandle_;
  Handle<render::GraphicsProgram> graphicsProgramHandle_;
  PipelineCreateInfo pipelineCreateInfo_;
  const Pipeline* pipeline_{};
  asset::ShaderResource matParamUboSchema_;
  Buffer matParamsUbo_;
  std::byte* matParamsUboData_{};
  DescriptorSet matParamsDescriptorSet_;

  static PipelineColorBlendStateCreateInfo colorBlendStateFromPreset(BlendingPreset preset);
};

} // namespace aur::render