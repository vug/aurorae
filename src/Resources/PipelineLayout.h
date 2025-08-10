#pragma once

#include <vector>

#include "../Utils.h"
#include "../VulkanWrappers.h"
#include "VulkanResource.h"

namespace aur {

class DescriptorSetLayout;
class PipelineLayout;

struct PushConstantsInfo {
  const PipelineLayout& pipelineLayout;
  std::vector<ShaderStageType> stages;
  u32 sizeBytes{};
  const void* data{};
  u32 offset{};
};

struct PushConstant {
  std::vector<ShaderStageType> stages;
  u32 size;
};

struct PipelineLayoutCreateInfo {
  // Pipeline layout does not own descriptor set layouts, just refers to them
  std::vector<const DescriptorSetLayout*> descriptorSetLayouts;
  std::vector<PushConstant> pushConstants;
};

class PipelineLayout
    : public VulkanResource<PipelineLayout, VkPipelineLayout, PipelineLayoutCreateInfo, VkDevice> {
public:
  PipelineLayout() = default;
  PipelineLayout(VkDevice device, const PipelineLayoutCreateInfo& createInfo);
  ~PipelineLayout();

  PipelineLayout(PipelineLayout&& other) noexcept = default;
  PipelineLayout& operator=(PipelineLayout&& other) noexcept = default;

private:
  friend class VulkanResource<PipelineLayout, VkPipelineLayout, PipelineLayoutCreateInfo, VkDevice>;
  static VkPipelineLayout createImpl(PipelineLayout* self, const PipelineLayoutCreateInfo& createInfo,
                                     const std::tuple<VkDevice>& context);
  // The destroyer function called by the base class.
  void destroyImpl() const;
};

} // namespace aur