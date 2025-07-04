// clang-format off
#include <volk/volk.h>
#include "Renderer.h"
#include <VulkanMemoryAllocator/vk_mem_alloc.h>
// clang-format on

#include <array>

#include "Logger.h"
#include "Pipeline.h"
#include "Resources/Allocator.h"
#include "Resources/DescriptorPool.h"
#include "Resources/DescriptorSet.h"
#include "Utils.h"
#include "Vertex.h"
#include "VulkanWrappers.h"
#include "asset/Mesh.h"

namespace aur {

Renderer::Renderer(GLFWwindow* window, const char* appName, u32 initialWidth, u32 initialHeight)
    : vulkanContext_(window, appName)
    , swapchain_(vulkanContext_.getVkbDevice(), initialWidth, initialHeight)
    , currentWidth_(initialWidth)
    , currentHeight_(initialHeight) {

  // Command Pool
  const VkCommandPoolCreateInfo cmdPoolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = 0,
      .queueFamilyIndex = vulkanContext_.getGraphicsQueueFamilyIndex(),
  };
  VK(vkCreateCommandPool(vulkanContext_.getDevice(), &cmdPoolInfo, nullptr, &commandPool_));
  log().trace("Renderer main command pool created.");

  const VkCommandPoolCreateInfo cmdPoolOneShotInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = vulkanContext_.getGraphicsQueueFamilyIndex(),
  };
  VK(vkCreateCommandPool(vulkanContext_.getDevice(), &cmdPoolOneShotInfo, nullptr, &commandPoolOneShot_));
  log().trace("Renderer one shot command pool created.");

  // Command Buffers
  const VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  VK(vkAllocateCommandBuffers(vulkanContext_.getDevice(), &allocInfo, &commandBuffer_));
  const VkCommandBufferAllocateInfo allocInfoOneShot{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPoolOneShot_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  VK(vkAllocateCommandBuffers(vulkanContext_.getDevice(), &allocInfoOneShot, &commandBufferOneShot_));
  log().trace("Renderer main and one-shot command buffers are created/allocated.");

  const DescriptorPoolCreateInfo descPoolInfo{
      .maxSets = 512,
      .poolSizes = {
          {DescriptorType::UniformBuffer, 512}, // Many UBOs for per-frame, per-object data
          // {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024}, // Many textures
          // {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256},          // Some storage buffers for compute/large data
          // {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256},           // If you separate images/samplers
          // {VK_DESCRIPTOR_TYPE_SAMPLER, 64},                  // Reused samplers
          // {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 32},            // For render targets or compute images
          // Add other types as needed (e.g., INVOCATION_GRAPHICS_NV, ACCELERATION_STRUCTURE_KHR)
      }};
  descriptorPool_ = DescriptorPool(getDevice(), descPoolInfo);

  constexpr VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  for (auto& semaphore : imageAvailableSemaphores_)
    VK(vkCreateSemaphore(vulkanContext_.getDevice(), &semaphoreInfo, nullptr, &semaphore));
  for (auto& semaphore : renderFinishedSemaphores_)
    VK(vkCreateSemaphore(vulkanContext_.getDevice(), &semaphoreInfo, nullptr, &semaphore));

  constexpr VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                        .flags = VK_FENCE_CREATE_SIGNALED_BIT};
  VK(vkCreateFence(vulkanContext_.getDevice(), &fenceInfo, nullptr, &inFlightFence_));
  log().trace("Renderer sync objects created.");

  createSwapchainDepthResources(); // Create the depth buffer for depth attachment to swapchain image
  createPerFrameDataResources();

  log().trace("Renderer initialized.");
}

Renderer::~Renderer() {
  // Ensure GPU is idle before destroying resources
  deviceWaitIdle();

  cleanupSwapchainDepthResources(); // Destroy depth buffer

  // Clean up sync objects
  for (auto& semaphore : imageAvailableSemaphores_) {
    if (semaphore != VK_NULL_HANDLE)
      vkDestroySemaphore(vulkanContext_.getDevice(), semaphore, nullptr);
    semaphore = VK_NULL_HANDLE;
  }
  for (auto& semaphore : renderFinishedSemaphores_) {
    if (semaphore != VK_NULL_HANDLE)
      vkDestroySemaphore(vulkanContext_.getDevice(), semaphore, nullptr);
    semaphore = VK_NULL_HANDLE;
  }
  if (inFlightFence_ != VK_NULL_HANDLE)
    vkDestroyFence(vulkanContext_.getDevice(), inFlightFence_, nullptr);
  inFlightFence_ = VK_NULL_HANDLE;

  if (commandPool_ != VK_NULL_HANDLE) {
    // Command buffers allocated from this pool are implicitly freed
    vkDestroyCommandPool(vulkanContext_.getDevice(), commandPool_, nullptr);
    commandPool_ = VK_NULL_HANDLE;
    commandBuffer_ = VK_NULL_HANDLE;
  }
  if (commandPoolOneShot_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(vulkanContext_.getDevice(), commandPoolOneShot_, nullptr);
    commandPoolOneShot_ = VK_NULL_HANDLE;
    commandBufferOneShot_ = VK_NULL_HANDLE;
  }

  // Swapchain and VulkanContext are destroyed automatically by their destructors
  // Order: Swapchain (uses the device), VulkanContext (owns the device)
  log().info("Renderer destroyed.");
}

ShaderModule Renderer::createShaderModule(const ShaderModuleCreateInfo& createInfo,
                                          std::string_view debugName) const {
  ShaderModule obj{getDevice(), createInfo};
  setDebugName(obj, debugName);
  return obj;
}

Buffer Renderer::createBufferAndUploadData(const void* data, size_t size, BufferUsage usage,
                                           std::string_view debugName, std::optional<u32> itemCnt) const {
  // host visible, host coherent
  Buffer stagingBuffer = createBuffer(
      {
          .sizeBytes = size,
          .usages = {BufferUsage::TransferSrc},
          .memoryUsage = MemoryUsage::CpuOnly,
      },
      std::string{debugName} + " Staging");

  void* mappedData = stagingBuffer.map();
  memcpy(mappedData, data, size);
  stagingBuffer.unmap();

  // Create the device-local buffer (optimal for GPU access)
  Buffer deviceBuffer = createBuffer(
      {
          .sizeBytes = size,
          .usages = {usage, BufferUsage::TransferDst},
          .memoryUsage = MemoryUsage::GpuOnly,
          .itemCnt = itemCnt,
      },
      std::string{debugName} + " Device");

  constexpr VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  VK(vkBeginCommandBuffer(commandBufferOneShot_, &beginInfo));
  const VkBufferCopy copyRegion{
      .srcOffset = 0,
      .dstOffset = 0,
      .size = size,
  };
  vkCmdCopyBuffer(commandBufferOneShot_, stagingBuffer.getHandle(), deviceBuffer.getHandle(), 1, &copyRegion);
  VK(vkEndCommandBuffer(commandBufferOneShot_));

  VkFenceCreateInfo fenceInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
  };
  VkFence uploadFence;
  VK(vkCreateFence(getDevice(), &fenceInfo, nullptr, &uploadFence));
  const VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBufferOneShot_,
  };
  VK(vkQueueSubmit(vulkanContext_.getGraphicsQueue(), 1, &submitInfo, uploadFence));
  constexpr u64 oneSec = 1'000'000'000;
  VK(vkWaitForFences(getDevice(), 1, &uploadFence, VK_TRUE, oneSec));
  vkDestroyFence(getDevice(), uploadFence, nullptr);

  return deviceBuffer;
}

void Renderer::createPerFrameDataResources() {
  const std::vector<DescriptorSetLayoutBinding> bindings = {{
      .index = 0,
      .type = DescriptorType::UniformBuffer,
      .stages = {ShaderStage::Vertex},
  }};
  const DescriptorSetLayoutCreateInfo createInfo{.bindings = bindings};
  perFrameDescriptorSetLayout_ =
      createDescriptorSetLayout(createInfo, "Per-Frame Data Descriptor Set Layout");

  const DescriptorSetCreateInfo setCreateInfo{
      .layout = &perFrameDescriptorSetLayout_,
  };
  perFrameDescriptorSet_ = createDescriptorSet(setCreateInfo, "Per-Frame Data Descriptor Set");

  // Uniform Buffer
  BufferCreateInfo perFrameUniformCreateInto{.sizeBytes = sizeof(PerFrameData),
                                             .usages = {BufferUsage::Uniform},
                                             .memoryUsage = MemoryUsage::CpuToGpu};
  perFrameUniformBuffer_ = createBuffer(perFrameUniformCreateInto, "Per-Frame Data Uniform Buffer");

  // Now, link our buffer to the allocated descriptor set
  DescriptorBufferInfo bufferInfo{
      .buffer = perFrameUniformBuffer_,
      .offset = 0,
      .range = sizeof(PerFrameData),
  };
  WriteDescriptorSet write{
      .dstSet = perFrameDescriptorSet_,
      .binding = 0,
      .descriptorCnt = 1,
      .descriptorType = DescriptorType::UniformBuffer,
      .bufferInfo = &bufferInfo,
  };
  perFrameDescriptorSet_.update({write});
}

void Renderer::notifyResize(u32 newWidth, u32 newHeight) {
  framebufferWasResized_ = true;
  currentWidth_ = newWidth;
  currentHeight_ = newHeight;
  log().info("Renderer notified of resize: {}x{}", newWidth, newHeight);
}

void Renderer::internalRecreateSwapchain() {
  log().info("Recreating swapchain with dimensions: {}x{}", currentWidth_, currentHeight_);
  deviceWaitIdle(); // Ensure all operations on the
                    // old swapchain are complete
  swapchain_.recreate(vulkanContext_.getVkbDevice(), currentWidth_, currentHeight_);
  cleanupSwapchainDepthResources(); // Clean old depth resources
  createSwapchainDepthResources();  // Recreate depth resources with a new size
  framebufferWasResized_ = false;   // Handled
  swapchainIsStale_ = false;        // Handled
  log().info("Swapchain recreated.");
}

bool Renderer::beginFrame() {
  VK(vkWaitForFences(vulkanContext_.getDevice(), 1, &inFlightFence_, VK_TRUE, UINT64_MAX));

  if (framebufferWasResized_ || swapchainIsStale_) {
    internalRecreateSwapchain();
    // After recreation, it's often best to restart the frame acquisition
    // attempt. The next acquire operation might still say suboptimal once, but then it
    // should be fine. For simplicity, we'll try to acquire immediately. If it
    // fails with out-of-date, we'll handle it below.
  }

  VkResult result = vkAcquireNextImageKHR(vulkanContext_.getDevice(), swapchain_.getSwapchain(), UINT64_MAX,
                                          imageAvailableSemaphores_[currentInFlightImageIx_], VK_NULL_HANDLE,
                                          &currentSwapchainImageIx_);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    log().debug("Swapchain out of date/suboptimal during acquire. Recreating.");
    swapchainIsStale_ = true;    // Mark it explicitly
    internalRecreateSwapchain(); // Recreate immediately
    return false;                // Signal to skip rendering this frame and try again
  } else if (result != VK_SUCCESS) {
    log().fatal("Failed to acquire swap chain image: {}", vkResultToString(result));
  }

  // Only reset the fence if we are sure we will submit work that signals it
  VK(vkResetFences(vulkanContext_.getDevice(), 1, &inFlightFence_));

  // TODO(vug): introduce a scopedMap -> when it goes out of scope, it unmaps automatically.
  {
    void* data = perFrameUniformBuffer_.map();
    memcpy(data, &perFrameData, sizeof(perFrameData));
    perFrameUniformBuffer_.unmap();
  }

  // Reset the command pool (which resets all command buffers allocated from it)
  // more performant than to reset each command buffer individually
  VK(vkResetCommandPool(vulkanContext_.getDevice(), commandPool_, 0));

  constexpr VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      // Good practice for command buffers recorded each frame
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  VK(vkBeginCommandBuffer(commandBuffer_, &beginInfo));

  beginDebugLabel("Initial Swapchain Image Transition");
  // Transition depth image from UNDEFINED to DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  constexpr VkImageSubresourceRange depthSubresourceRange{
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
  constexpr VkImageSubresourceRange subresourceRange{
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
      .image = swapchain_.getImages()[currentSwapchainImageIx_],
      .subresourceRange = subresourceRange,
  };

  vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
                       &barrierToColorAttachment);
  endDebugLabel();

  // Begin dynamic rendering (which includes the clear operation)
  const VkRenderingAttachmentInfoKHR colorAttachmentInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
      .imageView = swapchain_.getImageViews()[currentSwapchainImageIx_],
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

  beginDebugLabel("Final Swapchain Image Transition for Presentation");
  // Transition swapchain image from COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
  constexpr VkImageSubresourceRange subresourceRange{
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
      .image = swapchain_.getImages()[currentSwapchainImageIx_],
      .subresourceRange = subresourceRange,
  };
  vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierToPresent);
  endDebugLabel();
  VK(vkEndCommandBuffer(commandBuffer_));

  const std::array<VkSemaphore, 1> waitSemaphores{imageAvailableSemaphores_[currentInFlightImageIx_]};
  constexpr std::array<VkPipelineStageFlags, 1> waitStages{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
  const std::array<VkSemaphore, 1> signalSemaphores{renderFinishedSemaphores_[currentInFlightImageIx_]};
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

  VK(vkQueueSubmit(vulkanContext_.getGraphicsQueue(), 1, &submitInfo, inFlightFence_));

  const VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = static_cast<u32>(signalSemaphores.size()),
      .pWaitSemaphores = signalSemaphores.data(),
      .swapchainCount = 1,
      .pSwapchains = &swapchain_.getSwapchain(),
      .pImageIndices = &currentSwapchainImageIx_,
  };

  const VkResult result = vkQueuePresentKHR(vulkanContext_.getPresentQueue(),
                                            &presentInfo); // Use the present queue
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    log().debug("Swapchain out of date/suboptimal during present. Flagging for next frame.");
    swapchainIsStale_ = true; // Will be handled at the start of the next beginFrame
  } else if (result != VK_SUCCESS)
    log().fatal("Failed to present swap chain image: {}", static_cast<int>(result));

  currentInFlightImageIx_ = (currentInFlightImageIx_ + 1) % kMaxImagesInFlight;
}
void Renderer::beginDebugLabel(std::string_view label) const {
  constexpr f32 color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  beginDebugLabel(label, color);
}

void Renderer::beginDebugLabel(std::string_view label, const f32 color[4]) const {
  if constexpr (kBuildType == BuildType::Debug) {
    // TODO(vug): mechanism to ensure this is called from inside command buffer recording
    VkDebugUtilsLabelEXT vkLabel{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = label.data(),
        .color = {color[0], color[1], color[2], color[3]},
    };
    vkCmdBeginDebugUtilsLabelEXT(commandBuffer_, &vkLabel);
  }
}
void Renderer::endDebugLabel() const {
  if constexpr (kBuildType == BuildType::Debug) {
    // TODO(vug): mechanism to ensure this is called from inside command buffer recording, and after a
    //            beginLabel
    vkCmdEndDebugUtilsLabelEXT(commandBuffer_);
  }
}

void Renderer::bindDescriptorSet(const BindDescriptorSetInfo& bindInfo) const {
  const DescriptorSetLayout& layout1 =
      *bindInfo.pipelineLayout->getCreateInfo().descriptorSetLayouts[bindInfo.setNo];
  const DescriptorSetLayout& layout2 = *bindInfo.descriptorSet->getCreateInfo().layout;
  if (!layout1.isEqual(layout2) || !layout1.isCompatible(layout2))
    log().fatal("Incompatible pipeline layout and descriptor set's layout are not compatible.");

  const VkBindDescriptorSetsInfoKHR bindDescriptorSetsInfo{
      .sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
      .pNext = nullptr,
      .stageFlags = toVkFlags(bindInfo.stages),
      .layout = bindInfo.pipelineLayout->getHandle(),
      .firstSet = bindInfo.setNo,
      .descriptorSetCount = 1,
      .pDescriptorSets = &bindInfo.descriptorSet->getHandle(),
      .dynamicOffsetCount = 0,
      .pDynamicOffsets = nullptr,
  };
  vkCmdBindDescriptorSets2KHR(commandBuffer_, &bindDescriptorSetsInfo); // [issue #7]
}

void Renderer::bindPipeline(const Pipeline& pipeline, const PushConstantsInfo* pushConstantInfoOpt) const {
  // TODO(vug): introduce Renderer::boundPipeline member that keeps currently bound pipeline state, so that
  //            later bindDescriptorSet can use it. Maybe it can have two variants... if pipeline is not
  //            provided it'll use bound pipeline (?)
  beginDebugLabel("Bind Pipeline, Upload Push Constants");
  vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.getHandle());
  const BindDescriptorSetInfo perFrameBindDescriptorInfo{
      .pipelineLayout = &pipeline.getPipelineLayout(),
      .descriptorSet = &perFrameDescriptorSet_,
      .setNo = 0,
      .stages = {ShaderStage::Vertex},
  };
  bindDescriptorSet(perFrameBindDescriptorInfo);

  if (pushConstantInfoOpt) {
    VkPushConstantsInfoKHR /* [issue #7] */ vkPushConstantsInfo{
        .sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO_KHR,
        .pNext = nullptr,
        .layout = pipeline.getPipelineLayout().getHandle(),
        .stageFlags = toVkFlags(pushConstantInfoOpt->stages),
        .offset = 0,
        .size = pushConstantInfoOpt->sizeBytes,
        .pValues = pushConstantInfoOpt->data,
    };
    vkCmdPushConstants2KHR /* [issue #7] */ (commandBuffer_, &vkPushConstantsInfo);
  }
  endDebugLabel();
}

void Renderer::setDynamicPipelineState() const {
  beginDebugLabel("Set Dynamic Pipeline State");
  const VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(swapchain_.getImageExtent().width),
      .height = static_cast<float>(swapchain_.getImageExtent().height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(commandBuffer_, 0, 1, &viewport);
  const VkRect2D scissor{{0, 0}, swapchain_.getImageExtent()};
  vkCmdSetScissor(commandBuffer_, 0, 1, &scissor);
  endDebugLabel();
}

void Renderer::drawWithoutVertexInput(const Pipeline& pipeline, u32 vertexCnt,
                                      const PushConstantsInfo* pushConstantInfoOpt) const {
  bindPipeline(pipeline, pushConstantInfoOpt);
  setDynamicPipelineState();

  vkCmdDraw(commandBuffer_, vertexCnt, 1, 0, 0);
}
void Renderer::drawVertices(const Pipeline& pipeline, const Buffer& vertexBuffer,
                            const PushConstantsInfo* pushConstantInfoOpt) const {
  bindPipeline(pipeline, pushConstantInfoOpt);
  setDynamicPipelineState();

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(getCommandBuffer(), 0, 1, &vertexBuffer.getHandle(), &offset);
  const std::optional<u32>& vertexCnt = vertexBuffer.getCreateInfo().itemCnt;
  if (!vertexCnt.has_value())
    log().fatal("Vertex buffer item count is not set.");
  vkCmdDraw(commandBuffer_, vertexCnt.value(), 1, 0, 0);
}

void Renderer::drawIndexed(const Pipeline& pipeline, const Buffer& vertexBuffer, const Buffer& indexBuffer,
                           const PushConstantsInfo* pushConstantInfoOpt) const {
  bindPipeline(pipeline, pushConstantInfoOpt);
  setDynamicPipelineState();

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(getCommandBuffer(), 0, 1, &vertexBuffer.getHandle(), &offset);
  vkCmdBindIndexBuffer(getCommandBuffer(), indexBuffer.getHandle(), 0, VK_INDEX_TYPE_UINT32);
  const std::optional<u32>& indexCnt = indexBuffer.getCreateInfo().itemCnt;
  if (!indexCnt.has_value())
    log().fatal("Index buffer item count is not set.");
  vkCmdDrawIndexed(commandBuffer_, indexCnt.value(), 1, 0, 0, 0);
}

void Renderer::deviceWaitIdle() const {
  VK(vkDeviceWaitIdle(vulkanContext_.getDevice()));
}

void Renderer::createSwapchainDepthResources() {
  // TODO: Implement a robust format selection mechanism.
  // For now, hardcoding VK_FORMAT_D32_SFLOAT.
  // Common alternatives: VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM
  depthFormat_ = VK_FORMAT_D32_SFLOAT;
  VkExtent2D swapchainExtent = swapchain_.getImageExtent();

  const VkImageCreateInfo imageInfo{
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

  constexpr VmaAllocationCreateInfo allocInfo = {
      .usage = VMA_MEMORY_USAGE_GPU_ONLY, // Depth buffer is device local
      .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

  VK(vmaCreateImage(getAllocator().getHandle(), &imageInfo, &allocInfo, &depthImage_, &depthImageMemory_,
                    nullptr));

  const VkImageViewCreateInfo viewInfo{
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

  VK(vkCreateImageView(vulkanContext_.getDevice(), &viewInfo, nullptr, &depthImageView_));
  log().debug("Depth resources created (Format: {}).", static_cast<int>(depthFormat_));
}

void Renderer::cleanupSwapchainDepthResources() {
  if (depthImageView_ != VK_NULL_HANDLE)
    vkDestroyImageView(vulkanContext_.getDevice(), depthImageView_, nullptr);
  if (depthImage_ != VK_NULL_HANDLE) // Memory is freed with vmaDestroyImage
    vmaDestroyImage(getAllocator().getHandle(), depthImage_, depthImageMemory_);
  depthImageView_ = VK_NULL_HANDLE;
  depthImage_ = VK_NULL_HANDLE;
  depthImageMemory_ = VK_NULL_HANDLE;
}

render::Mesh Renderer::upload(Handle<asset::Mesh> meshHnd) const {
  render::Mesh rMesh;
  const asset::Mesh& aMesh = meshHnd.get();
  rMesh.vertexBuffer =
      createBufferAndUploadData(aMesh.vertices, BufferUsage::Vertex, aMesh.debugName + " Vertex Buffer");
  rMesh.indexBuffer =
      createBufferAndUploadData(aMesh.indices, BufferUsage::Index, aMesh.debugName + " Index Buffer");
  rMesh.asset = meshHnd;
  return rMesh;
}

render::Shader Renderer::upload(Handle<asset::Shader> shaderHnd) const {
  render::Shader rShader;
  const asset::Shader& aShader = shaderHnd.get();
  const ShaderModuleCreateInfo vertCreateInfo{.codeBlob = &aShader.getVertexBlob()};
  rShader.vertModule = createShaderModule(vertCreateInfo, aShader.getDebugName() + " Module");
  const ShaderModuleCreateInfo fragCreateInfo{.codeBlob = &aShader.getFragmentBlob()};
  rShader.fragModule = createShaderModule(fragCreateInfo, aShader.getDebugName() + " Module");
  return rShader;
}

void Renderer::setDebugNameWrapper(const VkDebugUtilsObjectNameInfoEXT& nameInfo) const {
  if constexpr (kBuildType == BuildType::Debug) {
    VK(vkSetDebugUtilsObjectNameEXT(getDevice(), &nameInfo));
  }
}

Buffer Renderer::createBuffer(const BufferCreateInfo& createInfo, std::string_view debugName) const {
  Buffer obj{getAllocator().getHandle(), createInfo};
  setDebugName(obj, debugName);
  return obj;
}
DescriptorSetLayout Renderer::createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo,
                                                        std::string_view debugName) const {
  DescriptorSetLayout obj{getDevice(), createInfo};
  setDebugName(obj, debugName);
  return obj;
}
DescriptorSet Renderer::createDescriptorSet(const DescriptorSetCreateInfo& createInfo,
                                            std::string_view debugName) const {
  DescriptorSet obj{getDevice(), descriptorPool_.getHandle(), createInfo};
  setDebugName(obj, debugName);
  return obj;
}
PipelineLayout Renderer::createPipelineLayout(const PipelineLayoutCreateInfo& createInfo,
                                              std::string_view debugName) const {
  PipelineLayout obj{getDevice(), createInfo};
  setDebugName(obj, debugName);
  return obj;
}

} // namespace aur