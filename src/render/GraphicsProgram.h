#pragma once

#include "../Handle.h"
#include "../Resources/DescriptorSetLayout.h"
#include "../Resources/ShaderModule.h"
#include "../asset/GraphicsProgram.h"

namespace aur {
class Renderer;
}

namespace aur::render {
class GraphicsProgram {
public:
  GraphicsProgram() = default;
  GraphicsProgram(const Renderer& renderer, Handle<asset::GraphicsProgram> asset);
  ~GraphicsProgram() = default;

  GraphicsProgram(const GraphicsProgram& other) = delete;
  GraphicsProgram(GraphicsProgram&& other) noexcept = default;
  GraphicsProgram& operator=(const GraphicsProgram& other) = delete;
  GraphicsProgram& operator=(GraphicsProgram&& other) noexcept = default;

  [[nodiscard]] const Handle<asset::GraphicsProgram>& getAssetHandle() const { return assetHandle_; }
  [[nodiscard]] const ShaderModule& getVertexShaderModule() const { return vertModule_; }
  [[nodiscard]] const ShaderModule& getFragmentShaderModule() const { return fragModule_; }
  [[nodiscard]] const std::vector<DescriptorSetLayout>& getDescriptorSetLayouts() const {
    return descriptorSetLayouts_;
  }
  [[nodiscard]] const std::vector<const DescriptorSetLayout*> getDescriptorSetLayoutRefs() const {
    std::vector<const DescriptorSetLayout*> layoutRefs;
    for (const auto& layout : descriptorSetLayouts_)
      layoutRefs.push_back(&layout);
    return layoutRefs;
  }

private:
  const Renderer* renderer_{};
  Handle<asset::GraphicsProgram> assetHandle_;

  ShaderModule vertModule_;
  ShaderModule fragModule_;
  std::vector<DescriptorSetLayout> descriptorSetLayouts_;
};
} // namespace aur::render