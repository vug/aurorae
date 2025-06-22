#pragma once

#include "../Utils.h"
#include "../VulkanWrappers.h"

namespace aur {

class VulkanContext;

struct AllocatorCreateInfo {
  u32 vulkanApiVersion{};
};

class Allocator {
public:
  Allocator() = default;
  Allocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
            const AllocatorCreateInfo& createInfo);
  ~Allocator();

  // move-only
  Allocator(const Allocator&) = delete;
  Allocator& operator=(const Allocator&) = delete;
  Allocator(Allocator&& other) noexcept;
  Allocator& operator=(Allocator&& other) noexcept;

  [[nodiscard]] inline bool isValid() const { return handle != VK_NULL_HANDLE; }

  const AllocatorCreateInfo createInfo;
  const VmaAllocator handle{VK_NULL_HANDLE};

private:
  friend class VulkanContext;
  void destroy();
  void invalidate();
};

} // namespace aur