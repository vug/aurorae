#pragma once

#include <unordered_map>

#include <glm/mat4x4.hpp>

#include "FileIO.h"
#include "Handle.h"
#include "Pipeline.h"
#include "Resources/Buffer.h"
#include "Resources/DescriptorPool.h"
#include "Resources/DescriptorSet.h"
#include "Resources/DescriptorSetLayout.h"
#include "Resources/PipelineCache.h"
#include "Resources/ShaderModule.h"
#include "Swapchain.h"
#include "VulkanContext.h"
#include "render/Material.h"
#include "render/Mesh.h"
#include "render/Shader.h"

struct GLFWwindow;

namespace aur {

class PipelineLayout;
struct PipelineLayoutCreateInfo;
struct PushConstantsInfo;
namespace asset {
class Mesh;
class Shader;
} // namespace asset

struct BindDescriptorSetInfo {
  const PipelineLayout* pipelineLayout{};
  const DescriptorSet* descriptorSet{};
  u32 setNo{};
  std::vector<ShaderStage> stages;
};

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
  [[nodiscard]] inline const DescriptorSet& getPerFrameDescriptorSet() const {
    return perFrameDescriptorSet_;
  }
  [[nodiscard]] inline const DescriptorSetLayout& getPerFrameDescriptorSetLayout() const {
    return perFrameDescriptorSetLayout_;
  }
  [[nodiscard]] inline const VkDevice& getVkDevice() const { return vulkanContext_.getDevice(); }
  [[nodiscard]] inline const Allocator& getAllocator() const { return vulkanContext_.getAllocator(); }
  [[nodiscard]] inline u32 getSwapchainImageCount() const { return swapchain_.getImageCount(); }
  [[nodiscard]] inline const VkFormat& getSwapchainColorImageVkFormat() const {
    return swapchain_.getImageFormat();
  }
  [[nodiscard]] inline const VkFormat& getSwapchainDepthImageFormat() const { return depthFormat_; }
  [[nodiscard]] inline const VkPipelineCache& getVkPipelineCache() const {
    return pipelineCache_.getHandle();
  }

  // Returns true if frame rendering can proceed.
  // Returns false if the swapchain was recreated (or another non-fatal issue) and the caller should skip
  // drawing and try the next frame. Must be called before any other draw commands
  bool beginFrame();
  // Must be called after draw commands
  void endFrame();

  void beginDebugLabel(std::string_view label) const;
  void beginDebugLabel(std::string_view label, const f32 color[4]) const;
  void endDebugLabel() const;
  inline void setClearColor(float r, float g, float b, float a = 1.0f) { clearColor_ = {r, g, b, a}; }
  void bindDescriptorSet(const BindDescriptorSetInfo& bindInfo) const;
  void bindPipeline(const Pipeline& pipeline, const PushConstantsInfo* pushConstantInfoOpt) const;
  void setDynamicPipelineState() const;

  void drawWithoutVertexInput(const Pipeline& pipeline, u32 vertexCnt,
                              const PushConstantsInfo* pushConstantInfoOpt = {}) const;
  void drawVertices(const Pipeline& pipeline, const Buffer& vertexBuffer,
                    const PushConstantsInfo* pushConstantInfoOpt) const;
  void drawIndexed(const Pipeline& pipeline, const Buffer& vertexBuffer, const Buffer& indexBuffer,
                   const PushConstantsInfo* pushConstantInfoOpt) const;
  void drawSpan(const render::DrawSpan drawSpan) const;

  void deviceWaitIdle() const;
  // Call this when the window framebuffer size has changed.
  void notifyResize(u32 newWidth, u32 newHeight);

  [[nodiscard]] Buffer createBuffer(const BufferCreateInfo& createInfo,
                                    std::string_view debugName = "") const;
  [[nodiscard]] Buffer createBufferAndUploadData(const void* data, size_t size, BufferUsage usage,
                                                 std::string_view debugName,
                                                 std::optional<u32> itemCnt = {}) const;
  template <std::ranges::contiguous_range TRange>
  [[nodiscard]] Buffer createBufferAndUploadData(const TRange& items, BufferUsage usage,
                                                 std::string_view debugName) const;
  [[nodiscard]] ShaderModule createShaderModule(const ShaderModuleCreateInfo& createInfo,
                                                std::string_view debugName = "") const;
  [[nodiscard]] DescriptorSetLayout createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo,
                                                              std::string_view debugName = "") const;
  [[nodiscard]] DescriptorSet createDescriptorSet(const DescriptorSetCreateInfo& createInfo,
                                                  std::string_view debugName = "") const;
  [[nodiscard]] PipelineLayout createPipelineLayout(const PipelineLayoutCreateInfo& createInfo,
                                                    std::string_view debugName = "") const;
  [[nodiscard]] const Pipeline* createOrGetPipeline(const PipelineCreateInfo& createInfo,
                                                    std::string_view debugName = "");
  template <typename TObject>
  void setDebugName(const TObject& obj, const std::string_view name) const;

  [[nodiscard]] Handle<render::Shader> uploadOrGet(Handle<asset::Shader> shaderHnd);
  [[nodiscard]] Handle<render::Material> uploadOrGet(Handle<asset::Material> materialHnd);
  [[nodiscard]] Handle<render::Mesh> uploadOrGet(Handle<asset::Mesh> meshHnd);

  [[nodiscard]] inline const render::Shader* get(Handle<render::Shader> handle) const {
    return &shaders_.at(handle);
  }
  [[nodiscard]] inline const render::Material* get(Handle<render::Material> handle) const {
    return &materials_.at(handle);
  }
  [[nodiscard]] inline const render::Mesh* get(Handle<render::Mesh> handle) const {
    return &meshes_.at(handle);
  }

  void onShaderAssetUpdated(Handle<asset::Shader> assetHnd);

  PerFrameData perFrameData;

private:
  void createPerFrameDataResources();

  // --- Swapchain & Framebuffer Resources ---
  // These are tied to the surface and swapping images.
  void internalRecreateSwapchain();
  void createSwapchainDepthResources();
  void cleanupSwapchainDepthResources();

  // to prevent inclusion of volk/vulkan headers
  void setDebugNameWrapper(const VkDebugUtilsObjectNameInfoEXT& nameInfo) const;

  // Context -> Swapchain needs to be created in that order.
  VulkanContext vulkanContext_;
  Swapchain swapchain_;

  VkCommandPool commandPool_{VK_NULL_HANDLE};
  VkCommandPool commandPoolOneShot_{VK_NULL_HANDLE};
  VkCommandBuffer commandBuffer_{VK_NULL_HANDLE};
  VkCommandBuffer commandBufferOneShot_{VK_NULL_HANDLE};
  PipelineCache pipelineCache_{};

  VkImage depthImage_{VK_NULL_HANDLE};
  VmaAllocation depthImageMemory_{VK_NULL_HANDLE};
  VkImageView depthImageView_{VK_NULL_HANDLE};
  VkFormat depthFormat_{VK_FORMAT_UNDEFINED};

  u32 currentInFlightImageIx_{};
  VkSemaphore imageAvailableSemaphores_[kMaxImagesInFlight] = {VK_NULL_HANDLE};
  VkSemaphore renderFinishedSemaphores_[kMaxImagesInFlight] = {VK_NULL_HANDLE};
  VkFence inFlightFence_{VK_NULL_HANDLE};
  u32 currentSwapchainImageIx_{};

  DescriptorPool descriptorPool_;
  DescriptorSetLayout perFrameDescriptorSetLayout_;
  DescriptorSet perFrameDescriptorSet_;
  Buffer perFrameUniformBuffer_;

  bool framebufferWasResized_{false};
  bool swapchainIsStale_{false}; // Combines suboptimal/out-of-date flags

  u32 currentWidth_;
  u32 currentHeight_;

  // Clear color can be set from Application or be fixed, default dark gray
  VkClearColorValue clearColor_{0.1f, 0.1f, 0.1f, 1.0f};
  // Default depth is 1.0 (far plane), stencil is 0
  VkClearDepthStencilValue clearDepthStencil_{1.0f, 0};

  std::unordered_map<PipelineCreateInfo, Pipeline> pipelineCacheMap_;
  std::vector<render::Shader> shaders_;
  std::vector<render::Material> materials_;
  std::vector<render::Mesh> meshes_;
  std::unordered_map<Handle<asset::Shader>, Handle<render::Shader>> shaderAssetToRenderHandleMap_;
  std::unordered_map<Handle<asset::Material>, Handle<render::Material>> materialAssetToRenderHandleMap_;
  std::unordered_map<Handle<asset::Mesh>, Handle<render::Mesh>> meshAssetToRenderHandleMap_;
};

template <std::ranges::contiguous_range TRange>
[[nodiscard]] Buffer Renderer::createBufferAndUploadData(const TRange& items, BufferUsage usage,
                                                         std::string_view debugName) const {
  using TItem = std::ranges::range_value_t<TRange>;
  const auto itemCnt = static_cast<u32>(std::ranges::size(items));
  const auto sizeBytes = itemCnt * sizeof(TItem);

  Buffer buffer = createBufferAndUploadData(items.data(), sizeBytes, usage, debugName, itemCnt);
  return buffer;
}

template <typename TObject>
void Renderer::setDebugName(const TObject& obj, const std::string_view name) const {
  VkObjectType objType = VK_OBJECT_TYPE_UNKNOWN;
  if constexpr (std::is_same_v<TObject, Buffer>)
    objType = VK_OBJECT_TYPE_BUFFER;
  else if constexpr (std::is_same_v<TObject, DescriptorPool>)
    objType = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
  else if constexpr (std::is_same_v<TObject, DescriptorSetLayout>)
    objType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
  else if constexpr (std::is_same_v<TObject, DescriptorSet>)
    objType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
  else if constexpr (std::is_same_v<TObject, PipelineLayout>)
    objType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
  else if constexpr (std::is_same_v<TObject, ShaderModule>)
    objType = VK_OBJECT_TYPE_SHADER_MODULE;
  else if constexpr (std::is_same_v<TObject, Pipeline>)
    objType = VK_OBJECT_TYPE_PIPELINE;
  else
    static_assert("Unsupported type TObject for setting debug name");
  VkDebugUtilsObjectNameInfoEXT nameInfo{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
      .objectType = objType,
      .objectHandle = reinterpret_cast<u64>(obj.getHandle()),
      .pObjectName = name.data(),
  };
  setDebugNameWrapper(nameInfo);
}

} // namespace aur