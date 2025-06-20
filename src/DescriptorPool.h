#pragma once

#include "VulkanWrappers.h"

namespace aur {

struct DescriptorPoolSize {
  DescriptorType type{};
  u32 count{};
};

struct DescriptorPoolCreateInfo {
  u32 maxSets{};
  std::vector<DescriptorPoolSize> poolSizes;
};

class DescriptorPool {
public:
  DescriptorPool() = default;
  DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo& createInfo);
  ~DescriptorPool();

  DescriptorPool(const DescriptorPool&) = delete;
  DescriptorPool& operator=(const DescriptorPool&) = delete;
  DescriptorPool(DescriptorPool&& other) noexcept;
  DescriptorPool& operator=(DescriptorPool&& other) noexcept;

  [[nodiscard]] inline bool isValid() const { return handle != VK_NULL_HANDLE; }

  const DescriptorPoolCreateInfo createInfo;
  const VkDescriptorPool handle{VK_NULL_HANDLE};

private:
  void invalidate();
  void destroy();

  VkDevice device_{VK_NULL_HANDLE};
};

} // namespace aur