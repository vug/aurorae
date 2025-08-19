#include "GraphicsProgram.h"

#include "../Logger.h"
#include "../Renderer.h"

namespace aur::render {

GraphicsProgram::GraphicsProgram(const Renderer& renderer, Handle<asset::GraphicsProgram> asset)
    : renderer_{&renderer}
    , assetHandle_{asset}
    , vertModule_{[this]() {
      const ShaderModuleCreateInfo vertCreateInfo{.spirv = &assetHandle_->getVertexBlob()};
      return renderer_->createShaderModule(vertCreateInfo, assetHandle_->getDebugName() + " Module");
    }()}
    , fragModule_{[this]() {
      const ShaderModuleCreateInfo fragCreateInfo{.spirv = &assetHandle_->getFragmentBlob()};
      return renderer_->createShaderModule(fragCreateInfo, assetHandle_->getDebugName() + " Module");
    }()}
    , descriptorSetLayouts_{[this]() {
      std::map<asset::SetNo, std::map<asset::BindingNo, DescriptorSetLayoutBinding>> layoutBindings;

      const asset::ShaderStageSchema& vertSchema = assetHandle_->getVertexSchema();
      for (const auto& [setNo, bindingMaps] : vertSchema.uniformsBuffers) {
        for (const auto& [bindingNo, uboSchema] : bindingMaps) {
          layoutBindings[setNo][bindingNo] = {
              .index = bindingNo,
              .type = DescriptorType::UniformBuffer,
              .descriptorCount = 1,
              .stages = {ShaderStageType::Vertex},
              .debugName = uboSchema.name,
          };
        }
      }

      const asset::ShaderStageSchema& fragSchema = assetHandle_->getFragmentSchema();
      for (const auto& [setNo, bindingMaps] : fragSchema.uniformsBuffers) {
        for (const auto& [bindingNo, uboSchema] : bindingMaps) {
          if (vertSchema.uniformsBuffers.contains(setNo) &&
              vertSchema.uniformsBuffers.at(setNo).contains(bindingNo)) {
            const asset::ShaderResource vertUboSchema = vertSchema.uniformsBuffers.at(setNo).at(bindingNo);
            if (vertUboSchema == uboSchema) {
              layoutBindings[setNo][bindingNo].stages.push_back(ShaderStageType::Fragment);
              continue;
            }
            log().fatal("The resources bound to set: {} and binding: {} by the vertex and fragment shaders "
                        "are different.",
                        setNo, bindingNo);
          }
          layoutBindings[setNo][bindingNo] = {
              .index = bindingNo,
              .type = DescriptorType::UniformBuffer,
              .descriptorCount = 1,
              .stages = {ShaderStageType::Fragment},
              .debugName = uboSchema.name,
          };
        }
      }

      std::vector<DescriptorSetLayout> layouts;
      for (const auto& setLayoutBindings : layoutBindings | std::views::values) {
        DescriptorSetLayoutCreateInfo createInfo{};
        for (const auto& layoutBinding : setLayoutBindings | std::views::values)
          createInfo.bindings.push_back(layoutBinding);
        layouts.emplace_back(renderer_->getVkDevice(), createInfo);
      }

      return layouts;
    }()} {}

} // namespace aur::render