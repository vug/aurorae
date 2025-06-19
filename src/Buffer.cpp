#include "Buffer.h"

#include <VulkanMemoryAllocator/vk_mem_alloc.h>

#include "Logger.h"

namespace aur {

Buffer::Buffer(VmaAllocator allocator, const BufferCreateInfo& bufferCreateInfo)
    : allocator_{allocator}
    , createInfo{bufferCreateInfo}
    , handle([this]() {
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

      VkBuffer hnd;
      VK(vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &hnd, &allocation_, nullptr));
      return hnd;
    }()) {}

Buffer::~Buffer() {
  destroy();
}

Buffer::Buffer(Buffer&& other) noexcept
    : allocator_{other.allocator_}
    , allocation_{other.allocation_}
    , handle{other.handle}
    , createInfo{other.createInfo} {
  other.invalidate();
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
  if (this != &other) {
    // Destroy the existing resource before taking ownership of the new one
    destroy();

    // Pilfer the resources from the other object
    allocator_ = other.allocator_;
    allocation_ = other.allocation_;
    const_cast<VkBuffer&>(handle) = other.handle;
    const_cast<BufferCreateInfo&>(createInfo) = other.createInfo;

    other.invalidate();
  }
  return *this;
}

void* Buffer::map() const {
  void* data{};
  VK(vmaMapMemory(allocator_, allocation_, &data));
  return data;
}

void Buffer::unmap() const {
  vmaUnmapMemory(allocator_, allocation_);
}
void Buffer::invalidate() {
  const_cast<VkBuffer&>(handle) = VK_NULL_HANDLE;
  allocation_ = VK_NULL_HANDLE;
  // not needed because not owned by Buffer
  allocator_ = VK_NULL_HANDLE;
}

void Buffer::destroy() {
  if (handle != VK_NULL_HANDLE && allocation_ != VK_NULL_HANDLE && allocator_ != VK_NULL_HANDLE)
    vmaDestroyBuffer(allocator_, handle, allocation_);
  invalidate();
}

} // namespace aur