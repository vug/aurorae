#pragma once

#include "../VulkanWrappers.h"
#include "VulkanResource.h"

namespace aur {

struct DescriptorPoolSize {
  DescriptorType type{};
  u32 count{};
};

struct DescriptorPoolCreateInfo {
  u32 maxSets{};
  std::vector<DescriptorPoolSize> poolSizes;
};

class DescriptorPool
    : public VulkanResource<DescriptorPool, VkDescriptorPool, DescriptorPoolCreateInfo, VkDevice> {
public:
  DescriptorPool() = default;
  DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo& createInfo);
  ~DescriptorPool();

  DescriptorPool(DescriptorPool&& other) noexcept = default;
  DescriptorPool& operator=(DescriptorPool&& other) noexcept = default;

private:
  friend class VulkanResource<DescriptorPool, VkDescriptorPool, DescriptorPoolCreateInfo, VkDevice>;

  static VkDescriptorPool createImpl(const DescriptorPoolCreateInfo& createInfo,
                                     const std::tuple<VkDevice>& context);
  void destroyImpl() const;
};

} // namespace aur