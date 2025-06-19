#pragma once

#include "Utils.h"

using VkDeviceSize = aur::u64;
using VkBufferUsageFlags = aur::u32;
enum VmaMemoryUsage : int;
FORWARD_DEFINE_VK_HANDLE(VmaAllocator)
FORWARD_DEFINE_VK_HANDLE(VmaAllocation)
FORWARD_DEFINE_VK_HANDLE(VkBuffer)

namespace aur {

struct BufferCreateInfo {
  VkDeviceSize size{};
  VkBufferUsageFlags usage{};
  VmaMemoryUsage memoryUsage{}; // VMA_MEMORY_USAGE_UNKNOWN
};

class Buffer {
public:
  // Default constructor for an empty buffer
  Buffer() = default;
  // Constructor to create and allocate the buffer
  Buffer(VmaAllocator allocator, const BufferCreateInfo& createInfo);
  // Destructor to clean up resources
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
  void invalidate();
  void destroy();
};

} // namespace aur