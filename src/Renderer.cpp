#include <volk/volk.h> // For Vulkan functions
#define VMA_IMPLEMENTATION
#include <VulkanMemoryAllocator/vk_mem_alloc.h>

#include "Renderer.h"

#include <array>

#include "Logger.h"
#include "Utils.h"

namespace aur {

Renderer::Renderer(GLFWwindow* window, const char* appName, u32 initialWidth, u32 initialHeight)
    : vulkanContext_(window, appName)
    , vmaAllocator_{makeVmaAllocator()}
    , swapchain_(vulkanContext_.getVkbDevice(), initialWidth, initialHeight)
    , currentWidth_(initialWidth)
    , currentHeight_(initialHeight) {
  createCommandPool();
  allocateCommandBuffer();
  createSyncObjects();
  createDepthResources();   // Create depth buffer
  createTrianglePipeline(); // Create the triangle pipeline
  createCubePipeline();     // Create the cube pipeline
  log().debug("Renderer initialized.");
}

Renderer::~Renderer() {
  // Ensure GPU is idle before destroying resources
  if (vulkanContext_.getDevice() != VK_NULL_HANDLE)
    vkDeviceWaitIdle(vulkanContext_.getDevice());

  cleanupCubePipeline();
  cleanupTrianglePipeline();
  cleanupDepthResources(); // Destroy depth buffer
  cleanupSyncObjects();
  cleanupCommandPool(); // Frees command buffers too

  if (vmaAllocator_ != VK_NULL_HANDLE) {
    vmaDestroyAllocator(vmaAllocator_);
    vmaAllocator_ = VK_NULL_HANDLE;
  }

  // Swapchain and VulkanContext are destroyed automatically by their destructors
  // Order: Swapchain (uses device), VulkanContext (owns device)
  log().info("Renderer destroyed.");
}

void Renderer::createCommandPool() {
  const VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = vulkanContext_.getGraphicsQueueFamilyIndex(),
  };
  if (vkCreateCommandPool(vulkanContext_.getDevice(), &poolInfo, nullptr, &commandPool_) != VK_SUCCESS)
    log().fatal("Failed to create command pool!");
}

void Renderer::allocateCommandBuffer() {
  const VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  if (vkAllocateCommandBuffers(vulkanContext_.getDevice(), &allocInfo, &commandBuffer_) != VK_SUCCESS)
    log().fatal("Failed to allocate command buffer!");
}

void Renderer::createSyncObjects() {
  const VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  const VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                    .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  if (vkCreateSemaphore(vulkanContext_.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore_) !=
          VK_SUCCESS ||
      vkCreateSemaphore(vulkanContext_.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore_) !=
          VK_SUCCESS ||
      vkCreateFence(vulkanContext_.getDevice(), &fenceInfo, nullptr, &inFlightFence_) != VK_SUCCESS)
    log().fatal("Failed to create synchronization objects!");
}

VkShaderModule Renderer::createShaderModule(BinaryBlob code) {
  const VkShaderModuleCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = code.size(),
      .pCode = reinterpret_cast<const u32*>(code.data()),
  };
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(vulkanContext_.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    log().fatal("Failed to create shader module!");
  return shaderModule;
}

void Renderer::createTrianglePipeline() {
  BinaryBlob vertShaderCode = readBinaryFile(pathJoin(kShadersFolder, "triangle.vert.spv").c_str());
  BinaryBlob fragShaderCode = readBinaryFile(pathJoin(kShadersFolder, "triangle.frag.spv").c_str());

  VkShaderModule vertShaderModule = createShaderModule(std::move(vertShaderCode));
  VkShaderModule fragShaderModule = createShaderModule(std::move(fragShaderCode));

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

  // For hardcoded vertices, vertex input state is empty
  const VkPipelineVertexInputStateCreateInfo vertexInputInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      // No vertex bindings or attributes
  };

  const VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  // Viewport and scissor will be dynamic
  const VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1, // Dynamic state will set this
      .scissorCount = 1,  // Dynamic state will set this
  };

  const VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE, // No culling for a 2D triangle
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };

  const VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
  };

  // No depth testing for 2D triangle
  const VkPipelineDepthStencilStateCreateInfo depthStencilState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_FALSE,  // Enable depth testing
      .depthWriteEnable = VK_FALSE, // Enable depth writes
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE};

  const VkPipelineColorBlendAttachmentState colorBlendAttachment{
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

  // Empty pipeline layout for now (no uniforms, push constants)
  const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
  };
  if (vkCreatePipelineLayout(vulkanContext_.getDevice(), &pipelineLayoutInfo, nullptr,
                             &trianglePipelineLayout_) != VK_SUCCESS)
    log().fatal("Failed to create triangle pipeline layout!");

  const std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  const VkPipelineDynamicStateCreateInfo dynamicStateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<u32>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data(),
  };

  // Dynamic Rendering Info
  const VkFormat colorAttachmentFormat = swapchain_.getImageFormat();
  const VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &colorAttachmentFormat,
      .depthAttachmentFormat = depthFormat_, // Must be compatible with vkCmdBeginRendering
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
      .layout = trianglePipelineLayout_,
      .renderPass = VK_NULL_HANDLE, // Must be null for dynamic rendering
      .subpass = 0,
  };

  if (vkCreateGraphicsPipelines(vulkanContext_.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                &triangleGraphicsPipeline_) != VK_SUCCESS)
    log().fatal("Failed to create triangle graphics pipeline!");

  // Shader modules can be destroyed after pipeline creation
  vkDestroyShaderModule(vulkanContext_.getDevice(), fragShaderModule, nullptr);
  vkDestroyShaderModule(vulkanContext_.getDevice(), vertShaderModule, nullptr);
  log().debug("Triangle graphics pipeline created.");
}

void Renderer::createCubePipeline() {
  BinaryBlob vertShaderCode = readBinaryFile(pathJoin(kShadersFolder, "cube.vert.spv").c_str());
  BinaryBlob fragShaderCode = readBinaryFile(pathJoin(kShadersFolder, "cube.frag.spv").c_str());

  VkShaderModule vertShaderModule = createShaderModule(std::move(vertShaderCode));
  VkShaderModule fragShaderModule = createShaderModule(std::move(fragShaderCode));

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

  const VkPipelineVertexInputStateCreateInfo vertexInputInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };

  const VkPipelineInputAssemblyStateCreateInfo inputAssembly{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  const VkPipelineViewportStateCreateInfo viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
  };

  const VkPipelineRasterizationStateCreateInfo rasterizer{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,            // Cull back faces
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, // Matches cube.vert winding
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };

  const VkPipelineMultisampleStateCreateInfo multisampling{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
  };

  const VkPipelineDepthStencilStateCreateInfo depthStencilState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS, // Standard depth test
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
  };

  const VkPipelineColorBlendAttachmentState colorBlendAttachment{
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

  // Empty pipeline layout for now (no uniforms, push constants)
  const VkPipelineLayoutCreateInfo pipelineLayoutInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  if (vkCreatePipelineLayout(vulkanContext_.getDevice(), &pipelineLayoutInfo, nullptr,
                             &cubePipelineLayout_) != VK_SUCCESS)
    log().fatal("Failed to create triangle pipeline layout!");

  const std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  const VkPipelineDynamicStateCreateInfo dynamicStateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<u32>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data(),
  };

  const VkFormat colorAttachmentFormat = swapchain_.getImageFormat();
  const VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &colorAttachmentFormat,
      .depthAttachmentFormat = depthFormat_,
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
      .layout = cubePipelineLayout_,
      .renderPass = VK_NULL_HANDLE,
      .subpass = 0,
  };

  if (vkCreateGraphicsPipelines(vulkanContext_.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                &cubeGraphicsPipeline_) != VK_SUCCESS)
    log().fatal("Failed to create cube graphics pipeline!");

  vkDestroyShaderModule(vulkanContext_.getDevice(), fragShaderModule, nullptr);
  vkDestroyShaderModule(vulkanContext_.getDevice(), vertShaderModule, nullptr);
  log().debug("Cube graphics pipeline created.");
}

void Renderer::cleanupCommandPool() {
  if (commandPool_ != VK_NULL_HANDLE) {
    // Command buffers allocated from this pool are implicitly freed
    vkDestroyCommandPool(vulkanContext_.getDevice(), commandPool_, nullptr);
    commandPool_ = VK_NULL_HANDLE;
    commandBuffer_ = VK_NULL_HANDLE; // It's freed with the pool
  }
}

void Renderer::cleanupSyncObjects() {
  if (imageAvailableSemaphore_ != VK_NULL_HANDLE)
    vkDestroySemaphore(vulkanContext_.getDevice(), imageAvailableSemaphore_, nullptr);
  if (renderFinishedSemaphore_ != VK_NULL_HANDLE)
    vkDestroySemaphore(vulkanContext_.getDevice(), renderFinishedSemaphore_, nullptr);
  if (inFlightFence_ != VK_NULL_HANDLE)
    vkDestroyFence(vulkanContext_.getDevice(), inFlightFence_, nullptr);
  imageAvailableSemaphore_ = VK_NULL_HANDLE;
  renderFinishedSemaphore_ = VK_NULL_HANDLE;
  inFlightFence_ = VK_NULL_HANDLE;
}

void Renderer::cleanupTrianglePipeline() {
  if (triangleGraphicsPipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(vulkanContext_.getDevice(), triangleGraphicsPipeline_, nullptr);
    triangleGraphicsPipeline_ = VK_NULL_HANDLE;
  }
  if (trianglePipelineLayout_ != VK_NULL_HANDLE) { // Only destroy if not shared by cube
    vkDestroyPipelineLayout(vulkanContext_.getDevice(), trianglePipelineLayout_, nullptr);
    trianglePipelineLayout_ = VK_NULL_HANDLE;
  }
}

void Renderer::cleanupCubePipeline() {
  if (cubeGraphicsPipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(vulkanContext_.getDevice(), cubeGraphicsPipeline_, nullptr);
    cubeGraphicsPipeline_ = VK_NULL_HANDLE;
  }
  if (cubePipelineLayout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(vulkanContext_.getDevice(), cubePipelineLayout_, nullptr);
    cubePipelineLayout_ = VK_NULL_HANDLE;
  }
}

void Renderer::notifyResize(u32 newWidth, u32 newHeight) {
  framebufferWasResized_ = true;
  currentWidth_ = newWidth;
  currentHeight_ = newHeight;
  log().info("Renderer notified of resize: {}x{}", newWidth, newHeight);
}

void Renderer::internalRecreateSwapchain() {
  log().info("Recreating swapchain with dimensions: {}x{}", currentWidth_, currentHeight_);
  vkDeviceWaitIdle(vulkanContext_.getDevice()); // Ensure all operations on the
                                                // old swapchain are complete
  swapchain_.recreate(vulkanContext_.getVkbDevice(), currentWidth_, currentHeight_);
  cleanupDepthResources();        // Clean old depth resources
  createDepthResources();         // Recreate depth resources with new size
  framebufferWasResized_ = false; // Handled
  swapchainIsStale_ = false;      // Handled
  log().info("Swapchain recreated.");
}

bool Renderer::beginFrame() {
  vkWaitForFences(vulkanContext_.getDevice(), 1, &inFlightFence_, VK_TRUE, UINT64_MAX);

  if (framebufferWasResized_ || swapchainIsStale_) {
    internalRecreateSwapchain();
    // After recreation, it's often best to restart the frame acquisition
    // attempt. The next acquire might still say suboptimal once, but then it
    // should be fine. For simplicity, we'll try to acquire immediately. If it
    // fails with out-of-date, we'll handle it below.
  }

  VkResult result = vkAcquireNextImageKHR(vulkanContext_.getDevice(), swapchain_.getSwapchain(), UINT64_MAX,
                                          imageAvailableSemaphore_, VK_NULL_HANDLE, &currentImageIndex_);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    log().debug("Swapchain out of date/suboptimal during acquire. Recreating.");
    swapchainIsStale_ = true;    // Mark it explicitly
    internalRecreateSwapchain(); // Recreate immediately
    return false;                // Signal to skip rendering this frame and try again
  } else if (result != VK_SUCCESS) {
    log().error("Failed to acquire swap chain image: {}", static_cast<int>(result));
    // This is a more serious error, could throw or try to recover
    return false;
  }

  // Only reset the fence if we are sure we will submit work that signals it
  vkResetFences(vulkanContext_.getDevice(), 1, &inFlightFence_);

  // Reset the command pool (which resets all command buffers allocated from it)
  // more performant than to reset each command buffer individually
  vkResetCommandPool(vulkanContext_.getDevice(), commandPool_, 0);

  const VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      // Good practice for command buffers recorded each frame
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  if (vkBeginCommandBuffer(commandBuffer_, &beginInfo) != VK_SUCCESS) {
    log().error("Failed to begin recording command buffer!");
    return false; // Can't proceed with this frame
  }

  // Transition depth image from UNDEFINED to DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  const VkImageSubresourceRange depthSubresourceRange{
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  const VkImageMemoryBarrier barrierToDepthAttachment{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask =
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = depthImage_,
      .subresourceRange = depthSubresourceRange,
  };

  vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                       0, 0, nullptr, 0, nullptr, 1, &barrierToDepthAttachment);

  // Transition swapchain image from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL
  const VkImageSubresourceRange subresourceRange{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  const VkImageMemoryBarrier barrierToColorAttachment{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = swapchain_.getImages()[currentImageIndex_],
      .subresourceRange = subresourceRange,
  };

  vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &barrierToColorAttachment);

  // Begin dynamic rendering (which includes the clear operation)
  const VkRenderingAttachmentInfoKHR colorAttachmentInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
      .imageView = swapchain_.getImageViews()[currentImageIndex_],
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = clearColor_,
  };

  const VkRenderingAttachmentInfoKHR depthAttachmentInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
      .imageView = depthImageView_,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,     // Clear depth at start of pass
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,   // Or DONT_CARE if not needed after
      .clearValue = {.depthStencil = {1.0f, 0}}, // Clear depth to 1.0
  };

  const VkRenderingInfoKHR renderingInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
      .renderArea = {{0, 0}, swapchain_.getImageExtent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentInfo,
      .pDepthAttachment = &depthAttachmentInfo,
      .pStencilAttachment = nullptr, // No stencil for now
  };
  vkCmdBeginRendering(commandBuffer_, &renderingInfo);

  return true; // Ready for drawing commands
}

void Renderer::endFrame() {
  vkCmdEndRendering(commandBuffer_); // End the dynamic rendering pass

  // Transition swapchain image from COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
  const VkImageSubresourceRange subresourceRange{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };
  const VkImageMemoryBarrier barrierToPresent{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = 0, // No specific access for present needed by app
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = swapchain_.getImages()[currentImageIndex_],
      .subresourceRange = subresourceRange,
  };
  vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierToPresent);

  if (vkEndCommandBuffer(commandBuffer_) != VK_SUCCESS)
    log().fatal("Failed to record command buffer!");

  const std::array<VkSemaphore, 1> waitSemaphores{imageAvailableSemaphore_};
  const std::array<VkPipelineStageFlags, 1> waitStages{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
  const std::array<VkSemaphore, 1> signalSemaphores{renderFinishedSemaphore_};
  const VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = static_cast<u32>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer_,
      .signalSemaphoreCount = static_cast<u32>(signalSemaphores.size()),
      .pSignalSemaphores = signalSemaphores.data(),
  };

  if (vkQueueSubmit(vulkanContext_.getGraphicsQueue(), 1, &submitInfo, inFlightFence_) != VK_SUCCESS)
    log().fatal("Failed to submit draw command buffer!");

  const VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = static_cast<u32>(signalSemaphores.size()),
      .pWaitSemaphores = signalSemaphores.data(),
      .swapchainCount = 1,
      .pSwapchains = &swapchain_.getSwapchain(),
      .pImageIndices = &currentImageIndex_,
  };

  VkResult result = vkQueuePresentKHR(vulkanContext_.getPresentQueue(),
                                      &presentInfo); // Use present queue
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    log().debug("Swapchain out of date/suboptimal during present. Flagging for next frame.");
    swapchainIsStale_ = true; // Will be handled at the start of the next beginFrame
  } else if (result != VK_SUCCESS)
    log().fatal("Failed to present swap chain image: {}", static_cast<int>(result));
}

void Renderer::drawNoVertexInput(VkCommandBuffer commandBuffer, VkPipeline pipeline, u32 vertexCnt) {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  const VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(swapchain_.getImageExtent().width),
      .height = static_cast<float>(swapchain_.getImageExtent().height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  const VkRect2D scissor{{0, 0}, swapchain_.getImageExtent()};
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdDraw(commandBuffer, vertexCnt, 1, 0, 0);
}

VmaAllocator Renderer::makeVmaAllocator() {
  VmaVulkanFunctions vmaVulkanFunctions = {
      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
  };
  VmaAllocatorCreateInfo allocatorInfo = {
      .physicalDevice = vulkanContext_.getPhysicalDevice(),
      .device = vulkanContext_.getDevice(),
      .pVulkanFunctions = &vmaVulkanFunctions,
      .instance = vulkanContext_.getInstance(),
      .vulkanApiVersion = VK_API_VERSION_1_3,
  };
  VmaAllocator vmaAllocator;
  if (vmaCreateAllocator(&allocatorInfo, &vmaAllocator) != VK_SUCCESS)
    log().fatal("Failed to create Vulkan Memory Allocator!");
  return vmaAllocator;
}

void Renderer::createDepthResources() {
  // TODO: Implement a robust format selection mechanism.
  // For now, hardcoding VK_FORMAT_D32_SFLOAT.
  // Common alternatives: VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM
  depthFormat_ = VK_FORMAT_D32_SFLOAT;
  VkExtent2D swapchainExtent = swapchain_.getImageExtent();

  VkImageCreateInfo imageInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = depthFormat_,
      .extent = {swapchainExtent.width, swapchainExtent.height, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VmaAllocationCreateInfo allocInfo = {.usage = VMA_MEMORY_USAGE_GPU_ONLY, // Depth buffer is device local
                                       .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

  if (vmaCreateImage(vmaAllocator_, &imageInfo, &allocInfo, &depthImage_, &depthImageMemory_, nullptr) !=
      VK_SUCCESS) {
    log().fatal("Failed to create depth image!");
  }

  VkImageViewCreateInfo viewInfo{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = depthImage_,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = depthFormat_,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

  if (vkCreateImageView(vulkanContext_.getDevice(), &viewInfo, nullptr, &depthImageView_) != VK_SUCCESS)
    log().fatal("Failed to create depth image view!");
  log().debug("Depth resources created (Format: {}).", static_cast<int>(depthFormat_));
}

void Renderer::cleanupDepthResources() {
  if (depthImageView_ != VK_NULL_HANDLE)
    vkDestroyImageView(vulkanContext_.getDevice(), depthImageView_, nullptr);
  if (depthImage_ != VK_NULL_HANDLE) // Memory is freed with vmaDestroyImage
    vmaDestroyImage(vmaAllocator_, depthImage_, depthImageMemory_);
  depthImageView_ = VK_NULL_HANDLE;
  depthImage_ = VK_NULL_HANDLE;
  depthImageMemory_ = VK_NULL_HANDLE;
  log().debug("Depth resources cleaned up.");
}
} // namespace aur