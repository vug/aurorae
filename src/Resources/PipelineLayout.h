#pragma once

#include <vector>

#include "../Utils.h"
#include "../VulkanWrappers.h"

namespace aur {

class DescriptorSetLayout;
class PipelineLayout;

struct PushConstantsInfo {
  const PipelineLayout& pipelineLayout;
  std::vector<ShaderStage> stages;
  u32 sizeBytes{};
  void* data{};
  u32 offset{};
};

struct PushConstant {
  std::vector<ShaderStage> stages;
  u32 size;
};

struct PipelineLayoutCreateInfo {
  // Pipeline layout does not own descriptor set layouts, just refers to them
  std::vector<const DescriptorSetLayout*> descriptorSetLayouts;
  std::vector<PushConstant> pushConstants;
};

class PipelineLayout {
public:
  PipelineLayout() = default;
  PipelineLayout(VkDevice device, const PipelineLayoutCreateInfo& createInfo);
  ~PipelineLayout();

  PipelineLayout(const PipelineLayout&) = delete;
  PipelineLayout& operator=(const PipelineLayout&) = delete;
  PipelineLayout(PipelineLayout&& other) noexcept;
  PipelineLayout& operator=(PipelineLayout&& other) noexcept;

  [[nodiscard]] const PipelineLayoutCreateInfo& getCreateInfo() const { return createInfo_; }
  [[nodiscard]] const VkPipelineLayout& getHandle() const { return handle_; }
  [[nodiscard]] inline bool isValid() const { return handle_ != VK_NULL_HANDLE; }

private:
  void invalidate();
  void destroy();

  VkDevice device_{VK_NULL_HANDLE};
  PipelineLayoutCreateInfo createInfo_;
  VkPipelineLayout handle_{VK_NULL_HANDLE};
};

} // namespace aur