#include "Buffer.h"

#include <VulkanMemoryAllocator/vk_mem_alloc.h>

#include "../Logger.h"

namespace aur {

Buffer::Buffer(VmaAllocator allocator, const BufferCreateInfo& bufferCreateInfo)
    : allocator_{allocator}
    , createInfo{bufferCreateInfo}
    , handle([this]() {
      const VkBufferCreateInfo bufferInfo{
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .size = createInfo.sizeBytes,
          .usage = toVkFlags(createInfo.usages),
          // For now, we'll stick to exclusive access from the graphics queue.
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      };
      const VmaAllocationCreateInfo allocInfo{
          .usage = static_cast<VmaMemoryUsage>(createInfo.memoryUsage),
      };

      VkBuffer hnd;
      VK(vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &hnd, &allocation_, nullptr));
      return hnd;
    }()) {}

Buffer::~Buffer() {
  destroy();
}

Buffer::Buffer(Buffer&& other) noexcept
    : allocator_(std::move(other.allocator_))
    , allocation_(std::move(other.allocation_))
    , createInfo(std::move(other.createInfo))
    , handle(std::move(other.handle)) {
  other.invalidate();
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
  if (this != &other) {
    // Destroy the existing resource before taking ownership of the new one
    destroy();

    // Pilfer the resources from the other object
    allocator_ = std::move(other.allocator_);
    allocation_ = std::move(other.allocation_);
    createInfo = std::move(other.createInfo);
    handle = std::move(other.handle);

    other.invalidate();
  }
  return *this;
}

void Buffer::destroy() {
  if (isValid() && allocation_ != VK_NULL_HANDLE)
    vmaDestroyBuffer(allocator_, handle, allocation_);
  invalidate();
}

void Buffer::invalidate() {
  handle = VK_NULL_HANDLE;
  allocation_ = VK_NULL_HANDLE;
}

void* Buffer::map() const {
  void* data{};
  VK(vmaMapMemory(allocator_, allocation_, &data));
  return data;
}

void Buffer::unmap() const {
  vmaUnmapMemory(allocator_, allocation_);
}

} // namespace aur