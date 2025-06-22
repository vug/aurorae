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

Pipeline Pipelines::createTrianglePipeline() const {
  PathBuffer vertexPath{pathJoin(kShadersFolder, "triangle.vert.spv")};
  PathBuffer fragmentPath{pathJoin(kShadersFolder, "triangle.frag.spv")};

  BinaryBlob vertShaderCode = readBinaryFile(vertexPath.c_str());
  BinaryBlob fragShaderCode = readBinaryFile(fragmentPath.c_str());

  VkShaderModule vertShaderModule = renderer_.createShaderModule(std::move(vertShaderCode));
  VkShaderModule fragShaderModule = renderer_.createShaderModule(std::move(fragShaderCode));

  const VkPipelineShaderStageCreateInfo vertShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertShaderModule,
      .pName = "main",
  };
  const VkPipelineShaderStageCreateInfo fragShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragShaderModule,
      .pName = "main",
  };
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

  // For hardcoded vertices, the vertex input state is empty
  constexpr VkPipelineVertexInputStateCreateInfo vertexInputInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      // No vertex bindings or attributes
  };

  constexpr VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  // Viewport and scissor will be dynamic
  constexpr VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1, // Dynamic state will set this
      .scissorCount = 1,  // Dynamic state will set this
  };

  constexpr VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE, // No culling for a 2D triangle
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };

  constexpr VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
  };

  // No depth testing for 2D triangle
  constexpr VkPipelineDepthStencilStateCreateInfo depthStencilState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_FALSE,  // Enable depth testing
      .depthWriteEnable = VK_FALSE, // Enable depth writes
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE};

  constexpr VkPipelineColorBlendAttachmentState colorBlendAttachment{
      .blendEnable = VK_FALSE, // No blending for opaque triangle
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT,
  };

  const VkPipelineColorBlendStateCreateInfo colorBlending{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment,
  };

  PipelineLayoutCreateInfo layoutCreateInfo{
      .descriptorSetLayouts = {&renderer_.getPerFrameDescriptorSetLayout()},
  };
  PipelineLayout pipelineLayout =
      renderer_.createPipelineLayout(layoutCreateInfo, "Triangle Pipeline Layout");

  constexpr std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                           VK_DYNAMIC_STATE_SCISSOR};
  const VkPipelineDynamicStateCreateInfo dynamicStateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<u32>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data(),
  };

  // Dynamic Rendering Info
  const VkFormat colorAttachmentFormat = renderer_.getSwapchainColorImageFormat();
  const VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &colorAttachmentFormat,
      .depthAttachmentFormat =
          renderer_.getSwapchainDepthImageFormat(), // Must be compatible with vkCmdBeginRendering
  };

  const VkGraphicsPipelineCreateInfo pipelineInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &pipelineRenderingCreateInfo, // Link dynamic rendering info
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
      .renderPass = VK_NULL_HANDLE, // Must be null for dynamic rendering
      .subpass = 0,
  };

  VkPipeline pipeline{VK_NULL_HANDLE};
  VK(vkCreateGraphicsPipelines(renderer_.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

  // Shader modules can be destroyed after pipeline creation
  vkDestroyShaderModule(renderer_.getDevice(), fragShaderModule, nullptr);
  vkDestroyShaderModule(renderer_.getDevice(), vertShaderModule, nullptr);
  log().debug("Triangle graphics pipeline created.");

  return {
      .vertexPath = std::move(vertexPath),
      .fragmentPath = std::move(fragmentPath),
      .pipeline = pipeline,
      .pipelineLayout = std::move(pipelineLayout),
  };
}

Pipeline Pipelines::createCubePipeline() const {
  PathBuffer vertexPath{pathJoin(kShadersFolder, "cube2.vert.spv")};
  PathBuffer fragmentPath{pathJoin(kShadersFolder, "cube2.frag.spv")};
  BinaryBlob vertShaderCode = readBinaryFile(vertexPath.c_str());
  BinaryBlob fragShaderCode = readBinaryFile(fragmentPath.c_str());

  VkShaderModule vertShaderModule = renderer_.createShaderModule(std::move(vertShaderCode));
  VkShaderModule fragShaderModule = renderer_.createShaderModule(std::move(fragShaderCode));

  const VkPipelineShaderStageCreateInfo vertShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vertShaderModule,
      .pName = "main",
  };
  const VkPipelineShaderStageCreateInfo fragShaderStageInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = fragShaderModule,
      .pName = "main",
  };
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

  constexpr VkPipelineVertexInputStateCreateInfo vertexInputInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };

  constexpr VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  constexpr VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
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

  PipelineLayout pipelineLayout = renderer_.createPipelineLayout(layoutCreateInfo, "Cube Pipeline Layout");

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

  vkDestroyShaderModule(renderer_.getDevice(), fragShaderModule, nullptr);
  vkDestroyShaderModule(renderer_.getDevice(), vertShaderModule, nullptr);
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
