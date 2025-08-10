#include "Pipeline.h"

#include <volk/volk.h>

#include "Logger.h"
#include "Renderer.h"
#include "Vertex.h"
#include "asset/GraphicsProgram.h"

namespace aur {
VkVertexInputBindingDescription toVkVertexInputBindingDescription(const VertexInputBindingDescription& desc) {
  const VkVertexInputBindingDescription vkBindingDescription{
      .binding = desc.binding, // The index of the binding in the array of bindings
      .stride = desc.stride,   // The byte stride between consecutive vertices
      .inputRate =
          static_cast<VkVertexInputRate>(desc.inputRate), // Move to the next vertex after each vertex
  };

  return vkBindingDescription;
}

VkVertexInputAttributeDescription
toVkVertexInputAttributeDescription(const VertexInputAttributeDescription& desc) {
  const VkVertexInputAttributeDescription vkAttributeDescription{
      .location = desc.location,
      .binding = desc.binding,
      .format = static_cast<VkFormat>(desc.format),
      .offset = desc.offset,
  };
  return vkAttributeDescription;
}

Pipeline::Pipeline(Renderer& renderer, const PipelineCreateInfo& createInfo)
    : createInfo_{createInfo}
    , renderer_{&renderer}
    , pipelineLayout_{[this]() {
      // WorldFromObject / Model matrix
      const PushConstant pushConstant{
          .stages = {ShaderStageType::Vertex},
          .size = sizeof(glm::mat4),
      };
      const PipelineLayoutCreateInfo layoutCreateInfo{
          .descriptorSetLayouts = {&renderer_->getPerFrameDescriptorSetLayout()},
          .pushConstants = {pushConstant},
      };

      return renderer_->createPipelineLayout(layoutCreateInfo, "Unlit Pipeline Layout");
    }()} {
  const render::GraphicsProgram& graphicsProgram = *createInfo.graphicsProgram;

  const VkPipelineShaderStageCreateInfo vertShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = graphicsProgram.getVertexShaderModule().getHandle(),
      .pName = "main",
  };
  const VkPipelineShaderStageCreateInfo fragShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = graphicsProgram.getFragmentShaderModule().getHandle(),
      .pName = "main",
  };
  std::array shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

  std::vector<VkVertexInputBindingDescription> vkInputBindingDescriptions;
  for (const auto& desc : Vertex::getVertexInputBindingDescription())
    vkInputBindingDescriptions.push_back(toVkVertexInputBindingDescription(desc));
  std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDescriptions;
  for (const auto& desc : Vertex::getVertexInputAttributeDescription())
    vkVertexInputAttributeDescriptions.push_back(toVkVertexInputAttributeDescription(desc));
  const VkPipelineVertexInputStateCreateInfo vertexInputInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = static_cast<u32>(vkInputBindingDescriptions.size()),
      .pVertexBindingDescriptions = vkInputBindingDescriptions.data(),
      .vertexAttributeDescriptionCount = static_cast<u32>(vkVertexInputAttributeDescriptions.size()),
      .pVertexAttributeDescriptions = vkVertexInputAttributeDescriptions.data(),
  };

  constexpr VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  constexpr VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1, // will be set by dynamic state
      .scissorCount = 1,  // will be set by dynamic state
  };

  const VkPipelineRasterizationStateCreateInfo rasterizationState = createInfo.rasterizationState.toVk();

  constexpr VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
  };

  const VkPipelineDepthStencilStateCreateInfo depthStencilState = createInfo.depthStencilState.toVk();

  const VkPipelineColorBlendStateCreateInfo colorBlending = createInfo.colorBlendState.toVk();

  constexpr std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                           VK_DYNAMIC_STATE_SCISSOR};
  const VkPipelineDynamicStateCreateInfo dynamicStateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<u32>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data(),
  };

  const VkFormat colorAttachmentFormat = renderer.getSwapchainColorImageVkFormat();
  const VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &colorAttachmentFormat,
      .depthAttachmentFormat = renderer.getSwapchainDepthImageFormat(),
  };

  const VkGraphicsPipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pipelineRenderingCreateInfo,
      .stageCount = static_cast<u32>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &inputAssembly,
      .pViewportState = &viewportState,
      .pRasterizationState = &rasterizationState,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depthStencilState,
      .pColorBlendState = &colorBlending,
      .pDynamicState = &dynamicStateInfo,
      .layout = pipelineLayout_.getHandle(),
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
  };

  // VkPipeline hnd{VK_NULL_HANDLE};
  VK(vkCreateGraphicsPipelines(renderer.getVkDevice(), renderer_->getVkPipelineCache(), 1, &pipelineInfo,
                               nullptr, &handle_));
}

Pipeline::~Pipeline() {
  if (handle_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(renderer_->getVkDevice(), handle_, nullptr);
    handle_ = VK_NULL_HANDLE;
  }
}

Pipeline::Pipeline(Pipeline&& other) noexcept
    : createInfo_{std::exchange(other.createInfo_, {})}
    , renderer_{std::exchange(other.renderer_, {})}
    , pipelineLayout_{std::exchange(other.pipelineLayout_, {})}
    , handle_{std::exchange(other.handle_, {})} {}

Pipeline& Pipeline::operator=(Pipeline&& other) noexcept {
  if (this == &other)
    return *this;

  destroy();

  createInfo_ = std::exchange(other.createInfo_, {});
  renderer_ = std::exchange(other.renderer_, {});
  pipelineLayout_ = std::exchange(other.pipelineLayout_, {});
  handle_ = std::exchange(other.handle_, {});

  return *this;
}

void Pipeline::invalidate() {
  handle_ = VK_NULL_HANDLE;
}

void Pipeline::destroy() {
  if (isValid())
    vkDestroyPipeline(renderer_->getVkDevice(), handle_, nullptr);
  invalidate();
}
} // namespace aur