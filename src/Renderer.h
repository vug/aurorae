#pragma once

#include "FileIO.h"
#include "Swapchain.h"
#include "VulkanContext.h"

struct GLFWwindow;
VK_DEFINE_HANDLE(VmaAllocator)

namespace aur {

class Renderer {
 public:
  Renderer(GLFWwindow* window, const char* appName, u32 initialWidth,
           u32 initialHeight);
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;

  VkCommandBuffer getCommandBuffer() const { return commandBuffer_; }
  u32 getCurrentImageIndex() const { return currentImageIndex_; }

  // Returns true if frame rendering can proceed.
  // Returns false if swapchain was recreated (or other non-fatal issue) and caller should skip drawing and try next frame.
  // Must be called before any other draw commands
  bool beginFrame();

  inline void setClearColor(float r, float g, float b, float a = 1.0f ) {
    clearColor_ = {r, g, b, a};
  }

  void draw(VkCommandBuffer commandBuffer);

  // Must be called after draw commands
  void endFrame();

  // Call this when the window framebuffer size has changed.
  void notifyResize(u32 newWidth, u32 newHeight);

 private:
  VmaAllocator makeVmaAllocator();
  void createCommandPool();
  void allocateCommandBuffer();
  void createSyncObjects();
  void internalRecreateSwapchain();

  VkShaderModule createShaderModule(BinaryBlob code);
  void createGraphicsPipeline();

  void cleanupSyncObjects();
  void cleanupCommandPool();  // Also frees command buffers
  void cleanupGraphicsPipeline();

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

  u32 currentImageIndex_{};
  bool framebufferWasResized_{false};
  bool swapchainIsStale_{false};  // Combines suboptimal/out-of-date flags

  u32 currentWidth_;
  u32 currentHeight_;

  // Clear color, can be set from Application or be fixed, default dark gray
  VkClearColorValue clearColor_{0.1f, 0.1f, 0.1f, 1.0f};
  // Default depth is 1.0 (far plane), stencil is 0
  VkClearDepthStencilValue clearDepthStencil_{1.0f, 0}; 
};

}  // namespace aur