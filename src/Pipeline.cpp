#include "Pipeline.h"

#include <volk/volk.h>

#include "Logger.h"
#include "Renderer.h"
#include "Vertex.h"
#include "asset/Shader.h"

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

Pipeline::Pipeline(const Renderer& renderer, const PipelineCreateInfo& createInfo)
    : createInfo_{createInfo}
    , renderer_{&renderer}
    , pipelineLayout_{[this]() {
      // WorldFromObject / Model matrix
      const PushConstant pushConstant{
          .stages = {ShaderStage::Vertex},
          .size = sizeof(glm::mat4),
      };
      const PipelineLayoutCreateInfo layoutCreateInfo{
          .descriptorSetLayouts = {&renderer_->getPerFrameDescriptorSetLayout()},
          .pushConstants = {pushConstant},
      };

      return renderer_->createPipelineLayout(layoutCreateInfo, "Unlit Pipeline Layout");
    }()} {
  const render::Shader shader = renderer.upload(createInfo.shader);

  const VkPipelineShaderStageCreateInfo vertShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = shader.vertModule.getHandle(),
      .pName = "main",
  };
  const VkPipelineShaderStageCreateInfo fragShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = shader.fragModule.getHandle(),
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

  constexpr VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,            // Cull back faces
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, // Matches cube.vert winding
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };

  constexpr VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
  };

  constexpr VkPipelineDepthStencilStateCreateInfo depthStencilState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS, // Standard depth test
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
  };

  constexpr VkPipelineColorBlendAttachmentState colorBlendAttachment{
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT,
  };

  const VkPipelineColorBlendStateCreateInfo colorBlending{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment,
  };

  constexpr std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                           VK_DYNAMIC_STATE_SCISSOR};
  const VkPipelineDynamicStateCreateInfo dynamicStateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<u32>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data(),
  };

  const VkFormat colorAttachmentFormat = renderer.getSwapchainColorImageFormat();
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
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = &depthStencilState,
      .pColorBlendState = &colorBlending,
      .pDynamicState = &dynamicStateInfo,
      .layout = pipelineLayout_.getHandle(),
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
  };

  // VkPipeline hnd{VK_NULL_HANDLE};
  VK(vkCreateGraphicsPipelines(renderer.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &handle_));
}

Pipeline::~Pipeline() {
  if (handle_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(renderer_->getDevice(), handle_, nullptr);
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
    vkDestroyPipeline(renderer_->getDevice(), handle_, nullptr);
  invalidate();
}
} // namespace aur