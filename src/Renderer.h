#pragma once

#include <array>
#include <string_view>
#include <vector>

#include "Swapchain.h"
#include "VulkanContext.h"

struct GLFWwindow;  // Forward declaration
VK_DEFINE_HANDLE(VmaAllocator)

namespace aur {

class Renderer {
 public:
  Renderer(GLFWwindow* window, std::string_view appName, uint32_t initialWidth,
           uint32_t initialHeight);
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;

  VkCommandBuffer getCommandBuffer() const { return commandBuffer_; }
  uint32_t getCurrentImageIndex() const { return currentImageIndex_; }

  // Returns true if frame rendering can proceed.
  // Returns false if swapchain was recreated (or other non-fatal issue) and
  // caller should skip drawing and try next frame.
  bool beginFrame();
  void draw(VkCommandBuffer commandBuffer);
  void endFrame();

  // Call this when the window framebuffer size has changed.
  void notifyResize(uint32_t newWidth, uint32_t newHeight);

 private:
  VmaAllocator makeVmaAllocator();
  void createCommandPool();
  void allocateCommandBuffer();
  void createSyncObjects();

  VkShaderModule createShaderModule(const std::vector<char>& code);
  void createGraphicsPipeline();

  void cleanupSyncObjects();
  void cleanupCommandPool();  // Also frees command buffers
  void cleanupGraphicsPipeline();

  void internalRecreateSwapchain();
  // Context -> Allocator -> Swapchain needs to be created in that order.
  VulkanContext vulkanContext_;
  VmaAllocator vmaAllocator_{VK_NULL_HANDLE};
  Swapchain swapchain_;

  VkCommandPool commandPool_{VK_NULL_HANDLE};
  VkCommandBuffer commandBuffer_{VK_NULL_HANDLE};

  VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
  VkPipeline graphicsPipeline_{VK_NULL_HANDLE};

  VkSemaphore imageAvailableSemaphore_{VK_NULL_HANDLE};
  VkSemaphore renderFinishedSemaphore_{VK_NULL_HANDLE};
  VkFence inFlightFence_{VK_NULL_HANDLE};

  uint32_t currentImageIndex_{};
  bool framebufferWasResized_{false};
  bool swapchainIsStale_{false};  // Combines suboptimal/out-of-date flags

  uint32_t currentWidth_;
  uint32_t currentHeight_;

  // Clear color, can be set from Application or be fixed
  std::array<float, 4> clearColor_{0.1f, 0.1f, 0.1f, 1.0f};
};

}  // namespace aur