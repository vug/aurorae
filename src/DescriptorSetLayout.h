#pragma once

#include <vector>

#include "Utils.h"
#include "VulkanWrappers.h"

namespace aur {

struct DescriptorSetLayoutBinding {
  u32 index;
  DescriptorType type;
  u32 descriptorCount{1};
  std::vector<ShaderStage> stages;

  bool operator==(const DescriptorSetLayoutBinding& other_binding) const = default;
};

struct DescriptorSetLayoutCreateInfo {
  std::vector<DescriptorSetLayoutBinding> bindings;
};

class DescriptorSetLayout {
public:
  DescriptorSetLayout() = default;
  DescriptorSetLayout(VkDevice device, const DescriptorSetLayoutCreateInfo& createInfo);
  ~DescriptorSetLayout();

  DescriptorSetLayout(const DescriptorSetLayout&) = delete;
  DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
  DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
  DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

  [[nodiscard]] inline bool isValid() const { return handle != VK_NULL_HANDLE; }

  const DescriptorSetLayoutCreateInfo createInfo;
  const VkDescriptorSetLayout handle{VK_NULL_HANDLE};

  bool isEqual(const DescriptorSetLayout& other) const { return handle == other.handle; }
  bool isCompatible(const DescriptorSetLayout& other) const;

private:
  VkDevice device_{VK_NULL_HANDLE};

  void invalidate();
  void destroy();
};
} // namespace aur
