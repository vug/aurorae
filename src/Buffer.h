#pragma once

#include <volk/volk.h>
#include <VulkanMemoryAllocator/vk_mem_alloc.h>

namespace aur {

struct BufferCreateInfo {
  VkDeviceSize size{};
  VkBufferUsageFlags usage{};
  VmaMemoryUsage memoryUsage{VMA_MEMORY_USAGE_UNKNOWN};
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

  // --- Accessors ---
  [[nodiscard]] VkBuffer getHandle() const { return buffer_; }
  [[nodiscard]] const BufferCreateInfo& getCreateInfo() const { return createInfo_; }
  [[nodiscard]] bool isValid() const { return buffer_ != VK_NULL_HANDLE; }

private:
  void destroy();

  VmaAllocator allocator_{VK_NULL_HANDLE};
  VmaAllocation allocation_{VK_NULL_HANDLE};
  VkBuffer buffer_{VK_NULL_HANDLE};
  BufferCreateInfo createInfo_{};
};

} // namespace aur