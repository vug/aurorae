#pragma once

#include "../Utils.h"
#include "../VulkanWrappers.h"

namespace aur {

struct BufferCreateInfo {
  u64 sizeBytes{};
  std::vector<BufferUsage> usages;
  VmaMemoryUsage memoryUsage{}; // default 0 is VMA_MEMORY_USAGE_UNKNOWN
};

class Buffer {
public:
  // Create an empty / invalid buffer
  Buffer() = default;
  // Create and allocate the buffer
  Buffer(VmaAllocator allocator, const BufferCreateInfo& createInfo);
  // Clean up resources and invalidate
  ~Buffer();

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  Buffer(Buffer&& other) noexcept;
  Buffer& operator=(Buffer&& other) noexcept;
  [[nodiscard]] bool isValid() const { return handle != VK_NULL_HANDLE; }

private:
  VmaAllocator allocator_{VK_NULL_HANDLE};
  VmaAllocation allocation_{VK_NULL_HANDLE};

public:
  const BufferCreateInfo createInfo{};
  const VkBuffer handle{VK_NULL_HANDLE};

  void* map() const;
  void unmap() const;

private:
  // nullify the handle (and other states)
  void invalidate();
  // destroy the resource
  void destroy();
};

} // namespace aur