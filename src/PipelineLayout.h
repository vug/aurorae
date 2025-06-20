#pragma once

#include <vector>

#include "Utils.h"
#include "VulkanWrappers.h"

namespace aur {

class DescriptorSetLayout;

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

  [[nodiscard]] inline bool isValid() const { return handle != VK_NULL_HANDLE; }

  const PipelineLayoutCreateInfo createInfo;
  const VkPipelineLayout handle{VK_NULL_HANDLE};

private:
  VkDevice device_{VK_NULL_HANDLE};

  void invalidate();
  void destroy();
};

} // namespace aur