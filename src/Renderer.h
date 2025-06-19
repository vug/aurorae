#pragma once

#include <glm/mat4x4.hpp>

#include "Allocator.h"
#include "Buffer.h"
#include "DescriptorSetLayout.h"
#include "FileIO.h"
#include "Swapchain.h"
#include "VulkanContext.h"

struct GLFWwindow;
FORWARD_DEFINE_VK_HANDLE(VmaAllocator)
FORWARD_DEFINE_VK_HANDLE(VmaAllocation)

namespace aur {

struct Pipeline;

struct PerFrameData {
  glm::mat4 viewFromObject{};
  glm::mat4 projectionFromView{};
  u64 frameIndex{};
};

class Renderer {
public:
  // This is the maximum value. The actual value can be 1 too.
  static constexpr u32 kMaxImagesInFlight = 2;

  Renderer(GLFWwindow* window, const char* appName, u32 initialWidth, u32 initialHeight);
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;

  [[nodiscard]] inline const VkCommandBuffer& getCommandBuffer() const { return commandBuffer_; }
  [[nodiscard]] inline u32 getCurrentImageIndex() const { return currentSwapchainImageIx_; }
  [[nodiscard]] inline const VkDescriptorSet& getPerFrameDescriptorSet() const {
    return perFrameDescriptorSet_;
  }
  [[nodiscard]] inline const DescriptorSetLayout& getPerFrameDescriptorSetLayout() const {
    return perFrameDescriptorSetLayout_;
  }
  [[nodiscard]] inline const VkDevice& getDevice() const { return vulkanContext_.getDevice(); }
  [[nodiscard]] inline u32 getSwapchainImageCount() const { return swapchain_.getImageCount(); }
  [[nodiscard]] inline const VkFormat& getSwapchainColorImageFormat() const {
    return swapchain_.getImageFormat();
  }
  [[nodiscard]] inline const VkFormat& getSwapchainDepthImageFormat() const { return depthFormat_; }

  // Returns true if frame rendering can proceed.
  // Returns false if the swapchain was recreated (or another non-fatal issue) and the caller should skip
  // drawing and try the next frame. Must be called before any other draw commands
  bool beginFrame();

  inline void setClearColor(float r, float g, float b, float a = 1.0f) { clearColor_ = {r, g, b, a}; }
  void bindDescriptorSet(VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet) const;
  void drawWithoutVertexInput(const Pipeline& pipeline, u32 vertexCnt,
                              const VkPushConstantsInfoKHR* /* [issue #7] */ pushConstantsInfo = {}) const;
  void deviceWaitIdle() const;

  // Must be called after draw commands
  void endFrame();

  // Call this when the window framebuffer size has changed.
  void notifyResize(u32 newWidth, u32 newHeight);

  [[nodiscard]] Buffer createBuffer(const BufferCreateInfo& createInfo) const;
  [[nodiscard]] VkShaderModule createShaderModule(BinaryBlob code) const;

private:
  void createPerFrameDataResources();

  // --- Swapchain & Framebuffer Resources ---
  // These are tied to the surface and swapping images.
  void internalRecreateSwapchain();
  void createSwapchainDepthResources();
  void cleanupSwapchainDepthResources();

  // Context -> Allocator -> Swapchain needs to be created in that order.
  VulkanContext vulkanContext_;
  // The Allocator must be declared before any resources (buffers, images) that use it
  Allocator allocator_;
  Swapchain swapchain_;

  VkCommandPool commandPool_{VK_NULL_HANDLE};
  VkCommandBuffer commandBuffer_{VK_NULL_HANDLE};

  VkImage depthImage_{VK_NULL_HANDLE};
  VmaAllocation depthImageMemory_{VK_NULL_HANDLE};
  VkImageView depthImageView_{VK_NULL_HANDLE};
  VkFormat depthFormat_{VK_FORMAT_UNDEFINED};

  u32 currentInFlightImageIx_{};
  VkSemaphore imageAvailableSemaphores_[kMaxImagesInFlight] = {VK_NULL_HANDLE};
  VkSemaphore renderFinishedSemaphores_[kMaxImagesInFlight] = {VK_NULL_HANDLE};
  VkFence inFlightFence_{VK_NULL_HANDLE};
  u32 currentSwapchainImageIx_{};

  DescriptorSetLayout perFrameDescriptorSetLayout_;
  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
  VkDescriptorSet perFrameDescriptorSet_{VK_NULL_HANDLE};
  Buffer perFrameUniformBuffer_;

  bool framebufferWasResized_{false};
  bool swapchainIsStale_{false}; // Combines suboptimal/out-of-date flags

  u32 currentWidth_;
  u32 currentHeight_;

  // Clear color can be set from Application or be fixed, default dark gray
  VkClearColorValue clearColor_{0.1f, 0.1f, 0.1f, 1.0f};
  // Default depth is 1.0 (far plane), stencil is 0
  VkClearDepthStencilValue clearDepthStencil_{1.0f, 0};
};

} // namespace aur