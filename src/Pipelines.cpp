#include "Pipelines.h"

#include <array>

#include <volk/volk.h>

#include "FileIO.h"
#include "Logger.h"
#include "Renderer.h"
#include "Resources/PipelineLayout.h"

namespace aur {
Pipelines::Pipelines(const Renderer& renderer)
    : renderer_(renderer) {}

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

Pipeline Pipelines::createUnlitPipeline() const {
  PathBuffer vertexPath{pathJoin(kShadersFolder, "unlit.vert.spv")};
  PathBuffer fragmentPath{pathJoin(kShadersFolder, "unlit.frag.spv")};
  ShaderModuleCreateInfo vertShaderModuleCreateInfo{
      .code = readBinaryFile(vertexPath.c_str()),
  };
  ShaderModuleCreateInfo fragShaderModuleCreateInfo{
      .code = readBinaryFile(fragmentPath.c_str()),
  };
  ShaderModule vertShaderModule = renderer_.createShaderModule(std::move(vertShaderModuleCreateInfo));
  ShaderModule fragShaderModule = renderer_.createShaderModule(std::move(fragShaderModuleCreateInfo));

  const VkPipelineShaderStageCreateInfo vertShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertShaderModule.handle,
      .pName = "main",
  };
  const VkPipelineShaderStageCreateInfo fragShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragShaderModule.handle,
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

  // WorldFromObject / Model matrix
  const PushConstant pushConstant{
      .stages = {ShaderStage::Vertex},
      .size = sizeof(glm::mat4),
  };
  PipelineLayoutCreateInfo layoutCreateInfo{
      .descriptorSetLayouts = {&renderer_.getPerFrameDescriptorSetLayout()},
      .pushConstants = {pushConstant},
  };

  PipelineLayout pipelineLayout = renderer_.createPipelineLayout(layoutCreateInfo, "Unlit Pipeline Layout");

  constexpr std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                           VK_DYNAMIC_STATE_SCISSOR};
  const VkPipelineDynamicStateCreateInfo dynamicStateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<u32>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data(),
  };

  const VkFormat colorAttachmentFormat = renderer_.getSwapchainColorImageFormat();
  const VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &colorAttachmentFormat,
      .depthAttachmentFormat = renderer_.getSwapchainDepthImageFormat(),
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
      .layout = pipelineLayout.handle,
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
  };

  VkPipeline pipeline{VK_NULL_HANDLE};
  VK(vkCreateGraphicsPipelines(renderer_.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
  log().debug("Cube graphics pipeline created.");

  return {
      .vertexPath = std::move(vertexPath),
      .fragmentPath = std::move(fragmentPath),
      .pipeline = pipeline,
      .pipelineLayout = std::move(pipelineLayout),
  };
}
void Pipelines::cleanupPipeline(Pipeline& pipeline) const {
  if (pipeline.pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(renderer_.getDevice(), pipeline.pipeline, nullptr);
    pipeline.pipeline = VK_NULL_HANDLE;
  }
}
} // namespace aur
