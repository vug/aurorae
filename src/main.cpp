// TODO(vug): introduce VMA (Vulkan Memory Allocator) for memory management
#define CROSS_PLATFORM_SURFACE_CREATION

#include <array>

#include <volk/volk.h>
// volk.h defines VK_NO_PROTOTYPES and includes necessary Vulkan core headers.
#include <glfw/glfw3.h>

#include "Utils.h"
#include "Logger.h"
#include "VulkanContext.h"
#include "Swapchain.h"


// GLFW framebuffer resize callback
static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

struct ApplicationState {
  // Flag for GLFW telling us the window has been resized  
  bool framebufferResized{};
  // Flag for Vulkan telling us the swapchain is out of date/suboptimal
  bool swapchainStale{}; 
};

int main() {
  aur::log_initialize(spdlog::level::trace);
  aur::log().info("Hi! Build Type: {}", static_cast<uint8_t>(aur::kBuildType));

  const uint32_t kWidth = 1024;
  const uint32_t kHeight = 768;
  const std::string_view kAppName = "Aurorae";

  ApplicationState appState;

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
  glfwSetWindowUserPointer(window, &appState);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);  

  // TODO(vug): put these in runMainLoop() function so that RAII objects
  // are destructed before GLFW destroys the window
  {
    aur::VulkanContext vulkanContext{window, kAppName};
    aur::Swapchain swapchain{vulkanContext.getVkbDevice(), kWidth, kHeight};

    const VkCommandPoolCreateInfo poolInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = vulkanContext.getGraphicsQueueFamilyIndex(),
    };
    VkCommandPool commandPool{};
    if (vkCreateCommandPool(vulkanContext.getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
      aur::log().fatal("Failed to create command pool!");

    const VkCommandBufferAllocateInfo allocInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(vulkanContext.getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS)
      aur::log().fatal("Failed to allocate command buffers!");

    // Synchronization objects
    const VkSemaphoreCreateInfo semaphoreInfo {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    if (vkCreateSemaphore(vulkanContext.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS)
        aur::log().fatal("Failed to create image available semaphore!");
    if (vkCreateSemaphore(vulkanContext.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
        aur::log().fatal("Failed to create render finished semaphore!");
    const VkFenceCreateInfo fenceInfo { 
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, 
      .flags = VK_FENCE_CREATE_SIGNALED_BIT // Create signaled for first frame
    }; 
    VkFence inFlightFence;
    if (vkCreateFence(vulkanContext.getDevice(), &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
        aur::log().fatal("Failed to create in-flight fence!");

    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      if (appState.framebufferResized || appState.swapchainStale) {
        appState.framebufferResized = false;
        appState.swapchainStale = false;

        int32_t currentWidth{};
        int32_t currentHeight{};
        glfwGetFramebufferSize(window, &currentWidth, &currentHeight);

        // Handle minimization: pause until window is restored
        while (currentWidth == 0 || currentHeight == 0) {
            aur::log().debug("Window is minimized (0x0), waiting for valid dimensions...");
            glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
            glfwWaitEvents();
        }
        aur::log().info("Swapchain recreation triggered. Recreating with new dimensions: {}x{}", currentWidth, currentHeight);
        swapchain.recreate(vulkanContext.getVkbDevice(), static_cast<uint32_t>(currentWidth), static_cast<uint32_t>(currentHeight));

        // After successful recreation, restart the frame acquisition process with the new swapchain.
        continue;
      }

      // Wait for the previous frame to finish before starting to record commands for the new one
      // This ensures the command buffer is no longer in use by the GPU.
      vkWaitForFences(vulkanContext.getDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);

      // Reset the command pool (which resets all command buffers allocated from it)
      if (vkResetCommandPool(vulkanContext.getDevice(), commandPool, 0) != VK_SUCCESS)
          aur::log().fatal("Failed to reset command pool!");
      vkResetFences(vulkanContext.getDevice(), 1, &inFlightFence);
      
      // Acquire an image from the swapchain.
      uint32_t imageIndex{};
      VkResult result = vkAcquireNextImageKHR(vulkanContext.getDevice(), swapchain.getSwapchain(), UINT64_MAX, 
        imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
          aur::log().debug("Swapchain out of date or suboptimal during acquire. Flagging recreation for next frame.");
          appState.swapchainStale = true;
          continue;
      } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
          aur::log().fatal("Failed to acquire swap chain image!");

      // Command buffer is already reset by vkResetCommandPool
      const VkCommandBufferBeginInfo beginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      };
      if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
          aur::log().fatal("Failed to begin recording command buffer!");

      const VkImageSubresourceRange subresourceRange {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      };

      // 1. Transition swapchain image from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL
      const VkImageMemoryBarrier barrierToColorAttachment {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0, // No prior app operations on this image in this CB, oldLayout is UNDEFINED
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // the clear via loadOp and any potential drawing) will write to the color attachment
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain.getImages()[imageIndex],
        .subresourceRange = subresourceRange,
      };
      vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Wait for acquire semaphore before this stage
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Before color attachment operations (like clear)
          0, 0, nullptr, 0, nullptr, 1, &barrierToColorAttachment);

      // 2. Begin dynamic rendering (which includes the clear operation)
      const VkRenderingAttachmentInfoKHR colorAttachmentInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = swapchain.getImageViews()[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Important to store the cleared result
        .clearValue = {{0.1f, 0.1f, 0.4f, 1.0f}}, // A pleasant dark blue
      };

      const VkRenderingInfoKHR renderingInfo {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = {{0, 0}, swapchain.getImageExtent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
      };

      vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);
      // No drawing commands are needed if we only want to clear.
      // The clear happens as part of vkCmdBeginRenderingKHR due to loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR.
      vkCmdEndRenderingKHR(commandBuffer);

      // 3. Transition swapchain image from COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
      const VkImageMemoryBarrier barrierToPresent {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // Ensure clear/store op is finished
        .dstAccessMask = 0, // No explicit access needed by app for PRESENT_SRC_KHR
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain.getImages()[imageIndex],
        .subresourceRange = subresourceRange,
      };
      vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // After color attachment operations
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // Before subsequent operations / presentation
          0, 0, nullptr, 0, nullptr, 1, &barrierToPresent);

      if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
          aur::log().fatal("Failed to record command buffer!");

      // Submit the command buffer
      const std::array<VkSemaphore, 1> waitSemaphores{imageAvailableSemaphore};
      const std::array<VkPipelineStageFlags, 1> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      const std::array<VkSemaphore, 1> signalSemaphores{renderFinishedSemaphore};
      const VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphores.data(),
        .pWaitDstStageMask = waitStages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
        .pSignalSemaphores = signalSemaphores.data(),
      };
      // No semaphores or fences needed here due to vkQueueWaitIdle and blocking acquire
      if (vkQueueSubmit(vulkanContext.getGraphicsQueue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
          aur::log().fatal("Failed to submit draw command buffer!");
      }

      // Present the image
      const VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()), // Wait for rendering to finish
        .pWaitSemaphores = signalSemaphores.data(),
        .swapchainCount = 1,
        .pSwapchains = &swapchain.getSwapchain(),
        .pImageIndices = &imageIndex,
      };
      result = vkQueuePresentKHR(vulkanContext.getGraphicsQueue(), &presentInfo);
      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
          aur::log().debug("Swapchain out of date or suboptimal during present. Flagging recreation for next frame.");
          appState.swapchainStale = true;
      } else if (result != VK_SUCCESS) {
          aur::log().fatal("Failed to present swap chain image!");
      }
    }

    vkDeviceWaitIdle(vulkanContext.getDevice());
    vkDestroySemaphore(vulkanContext.getDevice(), imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(vulkanContext.getDevice(), renderFinishedSemaphore, nullptr);
    vkDestroyFence(vulkanContext.getDevice(), inFlightFence, nullptr);
    vkDestroyCommandPool(vulkanContext.getDevice(), commandPool, nullptr);
    // Command buffer is implicitly freed with the pool
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  aur::log().info("Bye!");
  return 0;
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    (void)width; (void)height; // Parameters are not directly used, we query size in main loop
    auto appState = static_cast<ApplicationState*>(glfwGetWindowUserPointer(window));
    if (appState) {
        appState->framebufferResized = true;
    }
}
