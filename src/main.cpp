// TODO(vug): introduce VMA (Vulkan Memory Allocator) for memory management
#define CROSS_PLATFORM_SURFACE_CREATION

#include <array>

#include <volk/volk.h>
// explicit inclusion of vulkan.h is not necessary but is a good practice
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include "Logger.h"
#include "VulkanContext.h"
#include "Swapchain.h"

#include <glfw/glfw3.h>

int main() {
  aur::log_initialize(spdlog::level::trace);
  aur::log().info("Hi!");

  const uint32_t kWidth = 1024;
  const uint32_t kHeight = 768;
  const std::string_view kAppName = "Aurorae";

  // Initialize GLFW and create a GLFWwindow
  if (!glfwInit())
    aur::log().fatal("Failed to initialize GLFW");
  // Tell GLFW not to create an OpenGL/ES context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, kAppName.data(), nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    aur::log().fatal("Failed to create GLFW window");
  }

  // TODO(vug): put these in runMainLoop() function so that RAII objects
  // are destructed before GLFW destroys the window
  {
    aur::VulkanContext vulkanContext{window, kAppName};
    aur::Swapchain swapchain{vulkanContext, window};

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vulkanContext.getGraphicsQueueFamilyIndex();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool commandPool{};
    if (vkCreateCommandPool(vulkanContext.getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
      aur::log().fatal("Failed to create command pool!");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(vulkanContext.getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS)
      aur::log().fatal("Failed to allocate command buffers!");

    // Create a semaphore to signal when a swapchain image is available
    VkSemaphoreCreateInfo semaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkSemaphore imageAvailableSemaphore;
    if (vkCreateSemaphore(vulkanContext.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS)
        aur::log().fatal("Failed to create image available semaphore!");        
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      uint32_t imageIndex;
      // Acquire an image from the swapchain.
      // We use UINT64_MAX as the timeout to wait indefinitely until an image is available.
      // VK_NULL_HANDLE for semaphore and fence simplifies synchronization, relying on vkQueueWaitIdle later.
      VkResult result = vkAcquireNextImageKHR(vulkanContext.getDevice(), swapchain.getSwapchain(), UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

      if (result == VK_ERROR_OUT_OF_DATE_KHR) {
          aur::log().warn("Swapchain out of date. TODO: Implement swapchain recreation.");
          // In a real application, you'd recreate the swapchain here.
          // For this example, we'll skip rendering for this frame.
          continue;
      } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
          aur::log().fatal("Failed to acquire swap chain image!");
      }

      VkCommandBufferBeginInfo beginInfo{};
      beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We re-record this buffer each frame
      if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
          aur::log().fatal("Failed to begin recording command buffer!");

      // Define subresource range once, as it's the same for both barriers
      VkImageSubresourceRange subresourceRange{};
      subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      subresourceRange.baseMipLevel = 0;
      subresourceRange.levelCount = 1;
      subresourceRange.baseArrayLayer = 0;
      subresourceRange.layerCount = 1;

      // 1. Transition swapchain image from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL
      VkImageMemoryBarrier barrierToColorAttachment{};
      barrierToColorAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrierToColorAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      barrierToColorAttachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      barrierToColorAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrierToColorAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrierToColorAttachment.image = swapchain.getImages()[imageIndex];
      barrierToColorAttachment.subresourceRange = subresourceRange;
      barrierToColorAttachment.srcAccessMask = 0; // No prior operations on this image in this command buffer that need to be synchronized
      barrierToColorAttachment.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // the clear via loadOp and any potential drawing) will write to the color attachment

      vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,            // Before any operations
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Before color attachment operations (like clear)
          0, 0, nullptr, 0, nullptr, 1, &barrierToColorAttachment);

      // 2. Begin dynamic rendering (which includes the clear operation)
      VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
      colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
      colorAttachmentInfo.imageView = swapchain.getImageViews()[imageIndex];
      colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Important to store the cleared result
      colorAttachmentInfo.clearValue.color = {{0.1f, 0.1f, 0.4f, 1.0f}}; // A pleasant dark blue

      VkRenderingInfoKHR renderingInfo{};
      renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
      renderingInfo.renderArea.offset = {0, 0};
      renderingInfo.renderArea.extent = swapchain.getImageExtent();
      renderingInfo.layerCount = 1;
      renderingInfo.colorAttachmentCount = 1;
      renderingInfo.pColorAttachments = &colorAttachmentInfo;
      // No depth or stencil attachments for this simple clear

      vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);
      // No drawing commands are needed if we only want to clear.
      // The clear happens as part of vkCmdBeginRenderingKHR due to loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR.
      vkCmdEndRenderingKHR(commandBuffer);

      // 3. Transition swapchain image from COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
      VkImageMemoryBarrier barrierToPresent{};
      barrierToPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrierToPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      barrierToPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      barrierToPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrierToPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrierToPresent.image = swapchain.getImages()[imageIndex];
      barrierToPresent.subresourceRange = subresourceRange;
      barrierToPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Ensure clear/store op is finished
      barrierToPresent.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;    // Presentation engine will read it (safer)
                                                                    // Could be 0 if presentation engine access is implicitly handled

      vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // After color attachment operations
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // Before subsequent operations / presentation
          0, 0, nullptr, 0, nullptr, 1, &barrierToPresent);

      if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
          aur::log().fatal("Failed to record command buffer!");

      // Submit the command buffer
      std::array<VkSemaphore, 1> waitSemaphores{imageAvailableSemaphore};
      std::array<VkPipelineStageFlags, 1> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphores.data(),
        .pWaitDstStageMask = waitStages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
      };
      // No semaphores or fences needed here due to vkQueueWaitIdle and blocking acquire
      if (vkQueueSubmit(vulkanContext.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
          aur::log().fatal("Failed to submit draw command buffer!");
      }

      // Wait for the graphics queue to become idle. This ensures rendering is complete.
      vkQueueWaitIdle(vulkanContext.getGraphicsQueue());

      // Present the image
      VkPresentInfoKHR presentInfo{};
      presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      presentInfo.swapchainCount = 1;
      presentInfo.pSwapchains = &swapchain.getSwapchain();
      presentInfo.pImageIndices = &imageIndex;
      result = vkQueuePresentKHR(vulkanContext.getGraphicsQueue(), &presentInfo);
      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
          aur::log().warn("Swapchain out of date or suboptimal during present. TODO: Implement swapchain recreation.");
          // Handle swapchain recreation
      } else if (result != VK_SUCCESS) {
          aur::log().fatal("Failed to present swap chain image!");
      }

      // Reset command buffer for the next frame. This is safe because we waited for GPU idle.
      vkResetCommandBuffer(commandBuffer, 0);
    }

    // Wait for the device to be idle before destroying resources to ensure
    // the command buffer is no longer in use.
    vkDeviceWaitIdle(vulkanContext.getDevice());
    vkDestroySemaphore(vulkanContext.getDevice(), imageAvailableSemaphore, nullptr);
    vkDestroyCommandPool(vulkanContext.getDevice(), commandPool, nullptr);
    // Command buffer is implicitly freed with the pool
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  aur::log().info("Bye!");
  return 0;
}
