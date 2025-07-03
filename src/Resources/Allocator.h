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

  [[nodiscard]] const AllocatorCreateInfo& getCreateInfo() const { return createInfo_; }
  [[nodiscard]] const VmaAllocator& getHandle() const { return handle_; }
  [[nodiscard]] inline bool isValid() const { return handle_ != VK_NULL_HANDLE; }

private:
  friend class VulkanContext;
  void destroy();
  void invalidate();

  AllocatorCreateInfo createInfo_;
  VmaAllocator handle_{VK_NULL_HANDLE};
};

} // namespace aur