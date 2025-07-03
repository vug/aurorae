#pragma once

#include <vector>

#include "../VulkanWrappers.h"

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

  [[nodiscard]] const DescriptorSetLayoutCreateInfo& getCreateInfo() const { return createInfo_; }
  [[nodiscard]] const VkDescriptorSetLayout& getHandle() const { return handle_; }
  [[nodiscard]] inline bool isValid() const { return handle_ != VK_NULL_HANDLE; }

  bool isEqual(const DescriptorSetLayout& other) const { return handle_ == other.handle_; }
  bool isCompatible(const DescriptorSetLayout& other) const;

private:
  VkDevice device_{VK_NULL_HANDLE};
  DescriptorSetLayoutCreateInfo createInfo_;
  VkDescriptorSetLayout handle_{VK_NULL_HANDLE};

  void invalidate();
  void destroy();
};
} // namespace aur
