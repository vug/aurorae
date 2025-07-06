#pragma once

#include "../VulkanWrappers.h"
#include "VulkanResource.h"
#include <vector>

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

class DescriptorSetLayout : public VulkanResource<DescriptorSetLayout, VkDescriptorSetLayout,
                                                  DescriptorSetLayoutCreateInfo, VkDevice> {
public:
  DescriptorSetLayout() = default;
  DescriptorSetLayout(VkDevice device, const DescriptorSetLayoutCreateInfo& createInfo);
  ~DescriptorSetLayout();

  DescriptorSetLayout(const DescriptorSetLayout&) = delete;
  DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
  DescriptorSetLayout(DescriptorSetLayout&& other) noexcept = default;
  DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept = default;

  [[nodiscard]] bool isEqual(const DescriptorSetLayout& other) const { return handle_ == other.handle_; }
  [[nodiscard]] bool isCompatible(const DescriptorSetLayout& other) const;

private:
  friend class VulkanResource<DescriptorSetLayout, VkDescriptorSetLayout, DescriptorSetLayoutCreateInfo,
                              VkDevice>;

  static VkDescriptorSetLayout createImpl(DescriptorSetLayout* self,
                                          const DescriptorSetLayoutCreateInfo& createInfo,
                                          const std::tuple<VkDevice>& context);
  void destroyImpl() const;
};

} // namespace aur