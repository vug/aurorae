#include <volk/volk.h>  // For Vulkan functions

#include "Renderer.h"

#include <array>      // For std::array
#include <stdexcept>  // For std::runtime_error

#include "Logger.h"  // Already included via Renderer.h but good for clarity

namespace aur {

Renderer::Renderer(GLFWwindow* window, std::string_view appName,
                   uint32_t initialWidth, uint32_t initialHeight)
    : vulkanContext_(window, appName),
      swapchain_(vulkanContext_.getVkbDevice(), initialWidth, initialHeight),
      currentWidth_(initialWidth),
      currentHeight_(initialHeight) {
  try {
    createCommandPool();
    allocateCommandBuffer();
    createSyncObjects();
    log().info("Renderer initialized.");
  } catch (const std::exception& e) {
    // Cleanup what might have been created before throwing
    cleanupSyncObjects();
    cleanupCommandPool();  // This will also free command buffers if allocated
    // Swapchain and VulkanContext will be cleaned by their destructors
    log().error("Renderer initialization failed: {}", e.what());
    throw;  // Re-throw the exception
  }
}

Renderer::~Renderer() {
  // Ensure GPU is idle before destroying resources
  if (vulkanContext_.getDevice() != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(vulkanContext_.getDevice());
  }

  cleanupSyncObjects();
  cleanupCommandPool();  // Frees command buffers too
  // Swapchain and VulkanContext are destroyed automatically by their
  // destructors Order: Swapchain (uses device), VulkanContext (owns device)
  log().info("Renderer destroyed.");
}

void Renderer::createCommandPool() {
  const VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags =
          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,  // Allow individual
                                                            // command buffer
                                                            // reset
      .queueFamilyIndex = vulkanContext_.getGraphicsQueueFamilyIndex(),
  };
  if (vkCreateCommandPool(vulkanContext_.getDevice(), &poolInfo, nullptr,
                          &commandPool_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool!");
  }
}

void Renderer::allocateCommandBuffer() {
  const VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool_,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  if (vkAllocateCommandBuffers(vulkanContext_.getDevice(), &allocInfo,
                               &commandBuffer_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffer!");
  }
}

void Renderer::createSyncObjects() {
  const VkSemaphoreCreateInfo semaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  const VkFenceCreateInfo fenceInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  if (vkCreateSemaphore(vulkanContext_.getDevice(), &semaphoreInfo, nullptr,
                        &imageAvailableSemaphore_) != VK_SUCCESS ||
      vkCreateSemaphore(vulkanContext_.getDevice(), &semaphoreInfo, nullptr,
                        &renderFinishedSemaphore_) != VK_SUCCESS ||
      vkCreateFence(vulkanContext_.getDevice(), &fenceInfo, nullptr,
                    &inFlightFence_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create synchronization objects!");
  }
}

void Renderer::cleanupCommandPool() {
  if (commandPool_ != VK_NULL_HANDLE) {
    // Command buffers allocated from this pool are implicitly freed
    vkDestroyCommandPool(vulkanContext_.getDevice(), commandPool_, nullptr);
    commandPool_ = VK_NULL_HANDLE;
    commandBuffer_ = VK_NULL_HANDLE;  // It's freed with the pool
  }
}

void Renderer::cleanupSyncObjects() {
  if (imageAvailableSemaphore_ != VK_NULL_HANDLE)
    vkDestroySemaphore(vulkanContext_.getDevice(), imageAvailableSemaphore_,
                       nullptr);
  if (renderFinishedSemaphore_ != VK_NULL_HANDLE)
    vkDestroySemaphore(vulkanContext_.getDevice(), renderFinishedSemaphore_,
                       nullptr);
  if (inFlightFence_ != VK_NULL_HANDLE)
    vkDestroyFence(vulkanContext_.getDevice(), inFlightFence_, nullptr);
  imageAvailableSemaphore_ = VK_NULL_HANDLE;
  renderFinishedSemaphore_ = VK_NULL_HANDLE;
  inFlightFence_ = VK_NULL_HANDLE;
}

void Renderer::notifyResize(uint32_t newWidth, uint32_t newHeight) {
  framebufferWasResized_ = true;
  currentWidth_ = newWidth;
  currentHeight_ = newHeight;
  log().info("Renderer notified of resize: {}x{}", newWidth, newHeight);
}

void Renderer::internalRecreateSwapchain() {
  log().info("Recreating swapchain with dimensions: {}x{}", currentWidth_,
             currentHeight_);
  vkDeviceWaitIdle(vulkanContext_.getDevice());  // Ensure all operations on the
                                                 // old swapchain are complete
  swapchain_.recreate(vulkanContext_.getVkbDevice(), currentWidth_,
                      currentHeight_);
  framebufferWasResized_ = false;  // Handled
  swapchainIsStale_ = false;       // Handled
  log().info("Swapchain recreated.");
}

bool Renderer::beginFrame() {
  vkWaitForFences(vulkanContext_.getDevice(), 1, &inFlightFence_, VK_TRUE,
                  UINT64_MAX);

  if (framebufferWasResized_ || swapchainIsStale_) {
    internalRecreateSwapchain();
    // After recreation, it's often best to restart the frame acquisition
    // attempt. The next acquire might still say suboptimal once, but then it
    // should be fine. For simplicity, we'll try to acquire immediately. If it
    // fails with out-of-date, we'll handle it below.
  }

  VkResult result = vkAcquireNextImageKHR(
      vulkanContext_.getDevice(), swapchain_.getSwapchain(), UINT64_MAX,
      imageAvailableSemaphore_, VK_NULL_HANDLE, &currentImageIndex_);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    log().debug("Swapchain out of date/suboptimal during acquire. Recreating.");
    swapchainIsStale_ = true;     // Mark it explicitly
    internalRecreateSwapchain();  // Recreate immediately
    return false;  // Signal to skip rendering this frame and try again
  } else if (result != VK_SUCCESS) {
    log().error("Failed to acquire swap chain image: {}",
                static_cast<int>(result));
    // This is a more serious error, could throw or try to recover
    return false;
  }

  // Only reset the fence if we are sure we will submit work that signals it
  vkResetFences(vulkanContext_.getDevice(), 1, &inFlightFence_);

  // Reset command buffer (or pool if preferred)
  vkResetCommandBuffer(commandBuffer_, 0);

  const VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags =
          VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,  // Good practice for
                                                        // command buffers
                                                        // recorded each frame
  };
  if (vkBeginCommandBuffer(commandBuffer_, &beginInfo) != VK_SUCCESS) {
    log().error("Failed to begin recording command buffer!");
    return false;  // Can't proceed with this frame
  }

  // The actual drawing commands (like clearScreen) will be called by the
  // Application after beginFrame() returns true. For now, we can include the
  // initial transition here. Or, clearScreen itself can handle the first
  // transition. For this example, let clearScreen handle its necessary
  // transitions.

  return true;  // Ready for drawing commands
}

void Renderer::clearScreen(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                           const std::array<float, 4>& color) {
  const VkImageSubresourceRange subresourceRange{
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };

  // 1. Transition swapchain image from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL
  const VkImageMemoryBarrier barrierToColorAttachment{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = swapchain_.getImages()[imageIndex],
      .subresourceRange = subresourceRange,
  };
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &barrierToColorAttachment);

  // 2. Begin dynamic rendering (which includes the clear operation)
  const VkRenderingAttachmentInfoKHR colorAttachmentInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
      .imageView = swapchain_.getImageViews()[imageIndex],
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {{color[0], color[1], color[2], color[3]}},
  };
  const VkRenderingInfoKHR renderingInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
      .renderArea = {{0, 0}, swapchain_.getImageExtent()},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentInfo,
  };
  vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);
  // No drawing commands needed if we only want to clear.
  vkCmdEndRenderingKHR(
      commandBuffer);  // End rendering pass immediately after clear
}

void Renderer::endFrame() {
  // Note: clearScreen already called vkCmdEndRenderingKHR.
  // If user drawing was separate, vkCmdEndRenderingKHR would be here.

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
      .dstAccessMask = 0,  // No specific access for present needed by app
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = swapchain_.getImages()[currentImageIndex_],
      .subresourceRange = subresourceRange,
  };
  vkCmdPipelineBarrier(commandBuffer_,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrierToPresent);

  if (vkEndCommandBuffer(commandBuffer_) != VK_SUCCESS) {
    log().error("Failed to record command buffer!");
    // Potentially throw or handle error, for now, just log
    return;
  }

  const std::array<VkSemaphore, 1> waitSemaphores{imageAvailableSemaphore_};
  const std::array<VkPipelineStageFlags, 1> waitStages{
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
  const std::array<VkSemaphore, 1> signalSemaphores{renderFinishedSemaphore_};
  const VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
      .pWaitSemaphores = waitSemaphores.data(),
      .pWaitDstStageMask = waitStages.data(),
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer_,
      .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
      .pSignalSemaphores = signalSemaphores.data(),
  };

  if (vkQueueSubmit(vulkanContext_.getGraphicsQueue(), 1, &submitInfo,
                    inFlightFence_) != VK_SUCCESS) {
    log().error("Failed to submit draw command buffer!");
    // Potentially throw or handle error
    return;
  }

  const VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
      .pWaitSemaphores = signalSemaphores.data(),
      .swapchainCount = 1,
      .pSwapchains = &swapchain_.getSwapchain(),
      .pImageIndices = &currentImageIndex_,
  };

  VkResult result = vkQueuePresentKHR(vulkanContext_.getPresentQueue(),
                                      &presentInfo);  // Use present queue
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    log().debug(
        "Swapchain out of date/suboptimal during present. Flagging for next "
        "frame.");
    swapchainIsStale_ =
        true;  // Will be handled at the start of the next beginFrame
  } else if (result != VK_SUCCESS) {
    log().error("Failed to present swap chain image: {}",
                static_cast<int>(result));
    // Potentially throw or handle error
  }
}

}  // namespace aur