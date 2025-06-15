#include "Buffer.h"

#include "Logger.h"

namespace aur {

Buffer::Buffer(VmaAllocator allocator, const BufferCreateInfo& createInfo)
    : allocator_{allocator}
    , createInfo_{createInfo} {
  const VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = createInfo.size,
      .usage = createInfo.usage,
      // For now, we'll stick to exclusive access from the graphics queue.
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  const VmaAllocationCreateInfo allocInfo{
      .usage = createInfo.memoryUsage,
  };

  VkResult result = vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &buffer_, &allocation_, nullptr);
  if (result != VK_SUCCESS)
    log().fatal("Failed to create buffer! VMA error: %d", vkResultToString(result));
}

Buffer::~Buffer() {
  destroy();
}

Buffer::Buffer(Buffer&& other) noexcept
    : allocator_{other.allocator_}
    , allocation_{other.allocation_}
    , buffer_{other.buffer_}
    , createInfo_{other.createInfo_} {
  // Invalidate the other object so its destructor doesn't double-free
  other.allocator_ = VK_NULL_HANDLE;
  other.allocation_ = VK_NULL_HANDLE;
  other.buffer_ = VK_NULL_HANDLE;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
  if (this != &other) {
    // Destroy the existing resource before taking ownership of the new one
    destroy();

    // Pilfer the resources from the other object
    allocator_ = other.allocator_;
    allocation_ = other.allocation_;
    buffer_ = other.buffer_;
    createInfo_ = other.createInfo_;

    // Invalidate the other object
    other.allocator_ = VK_NULL_HANDLE;
    other.allocation_ = VK_NULL_HANDLE;
    other.buffer_ = VK_NULL_HANDLE;
  }
  return *this;
}

void* Buffer::map() const {
  void* data{};
  vmaMapMemory(allocator_, allocation_, &data);
  return data;
}

void Buffer::unmap() const {
  vmaUnmapMemory(allocator_, allocation_);
}

void Buffer::destroy() {
  if (buffer_ != VK_NULL_HANDLE && allocation_ != VK_NULL_HANDLE && allocator_ != VK_NULL_HANDLE) {
    vmaDestroyBuffer(allocator_, buffer_, allocation_);
    buffer_ = VK_NULL_HANDLE;
    allocation_ = VK_NULL_HANDLE;
    // We don't null out the allocator_ because it's owned by the Renderer,
    // but we can null the other handles to be safe.
  }
}

} // namespace aur