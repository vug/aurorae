// TODO(vug): introduce VMA (Vulkan Memory Allocator) for memory management
#define CROSS_PLATFORM_SURFACE_CREATION

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

    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      uint32_t imageIndex;
      // Acquire an image from the swapchain.
      // We use UINT64_MAX as the timeout to wait indefinitely until an image is available.
      // VK_NULL_HANDLE for semaphore and fence simplifies synchronization, relying on vkQueueWaitIdle later.
      VkResult result = vkAcquireNextImageKHR(vulkanContext.getDevice(), swapchain.getSwapchain(), UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);

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

      // 1. Transition swapchain image from UNDEFINED to TRANSFER_DST_OPTIMAL
      VkImageMemoryBarrier imageMemoryBarrier{};
      imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Image is in undefined layout after vkAcquireNextImageKHR
      imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      imageMemoryBarrier.image = swapchain.getImages()[imageIndex]; // Assumes Swapchain exposes its images
      imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
      imageMemoryBarrier.subresourceRange.levelCount = 1;
      imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
      imageMemoryBarrier.subresourceRange.layerCount = 1;
      imageMemoryBarrier.srcAccessMask = 0; // No access needed from undefined layout
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // We will write to it (clear)

      vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,  // Stage before which operations occur
          VK_PIPELINE_STAGE_TRANSFER_BIT,     // Stage at which transfer operations occur
          0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

      // 2. Clear the color image
      VkClearColorValue clearColor = {{0.1f, 0.1f, 0.4f, 1.0f}}; // A pleasant dark blue
      VkImageSubresourceRange clearRange = imageMemoryBarrier.subresourceRange; // Reuse the range
      vkCmdClearColorImage(commandBuffer, swapchain.getImages()[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &clearRange);

      // 3. Transition swapchain image from TRANSFER_DST_OPTIMAL to PRESENT_SRC_KHR
      imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Wait for the clear to finish
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;    // Presentation engine will read it

      vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,        // Stage after transfer operations
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,  // Stage before presentation
          0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

      if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
          aur::log().fatal("Failed to record command buffer!");

      // Submit the command buffer
      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &commandBuffer;
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
    vkDestroyCommandPool(vulkanContext.getDevice(), commandPool, nullptr);
    // Command buffer is implicitly freed with the pool
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  aur::log().info("Bye!");
  return 0;
}
