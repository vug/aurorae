#pragma once

#include "../Utils.h"
#include "../VulkanWrappers.h"
#include "VulkanResource.h"

namespace aur {

struct AllocatorCreateInfo {
  u32 vulkanApiVersion{}; // Vulkan API version
};

class Allocator : public VulkanResource<Allocator, VmaAllocator, AllocatorCreateInfo, VkInstance,
                                        VkPhysicalDevice, VkDevice> {
public:
  Allocator() = default;
  Allocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
            const AllocatorCreateInfo& createInfo);
  ~Allocator();

  Allocator(Allocator&& other) noexcept = default;
  Allocator& operator=(Allocator&& other) noexcept = default;

private:
  friend class VulkanResource<Allocator, VmaAllocator, AllocatorCreateInfo, VkInstance, VkPhysicalDevice,
                              VkDevice>;
  static VmaAllocator createImpl(const AllocatorCreateInfo& createInfo,
                                 const std::tuple<VkInstance, VkPhysicalDevice, VkDevice>& context);
  void destroyImpl() const;
};

} // namespace aur