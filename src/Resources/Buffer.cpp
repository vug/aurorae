#include "Buffer.h"

#include "../Logger.h"
#include <VulkanMemoryAllocator/vk_mem_alloc.h>
#include <cassert>

namespace aur {

// Thread-local storage for temporary per-thread `VmaAllocation`
static thread_local VmaAllocation tl_allocation = VK_NULL_HANDLE;

Buffer::Buffer(VmaAllocator allocator, const BufferCreateInfo& createInfo)
    : VulkanResource(createInfo, allocator) {
  // The VulkanResource base class automatically calls `createImpl` during initialization,
  // which assigns the thread-local `tl_allocation`.
  allocation_ = tl_allocation;
  tl_allocation = VK_NULL_HANDLE; // Clear thread-local allocation after use
  assert(allocation_ != VK_NULL_HANDLE && "Failed to assign VmaAllocation!");
}

Buffer::~Buffer() {
  destroy();
}

Buffer::Buffer(Buffer&& other) noexcept
    : VulkanResource(std::move(other))
    , allocation_(std::exchange(other.allocation_, VK_NULL_HANDLE)) {}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
  if (this != &other) {
    VulkanResource::operator=(std::move(other));
    allocation_ = std::exchange(other.allocation_, VK_NULL_HANDLE);
  }
  return *this;
}

VkBuffer Buffer::createImpl(Buffer* self, const BufferCreateInfo& createInfo,
                            const std::tuple<VmaAllocator>& context) {
  const VkBufferCreateInfo bufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = createInfo.sizeBytes,
      .usage = toVkFlags(createInfo.usages),
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  const VmaAllocationCreateInfo allocInfo = {
      .usage = static_cast<VmaMemoryUsage>(createInfo.memoryUsage),
  };

  VkBuffer bufferHandle = VK_NULL_HANDLE;

  // Use the thread-local variable to store allocation information.
  if (vmaCreateBuffer(std::get<VmaAllocator>(context), &bufferInfo, &allocInfo, &bufferHandle, &tl_allocation,
                      nullptr) != VK_SUCCESS) {
    log().fatal("Failed to create buffer!");
  }

  return bufferHandle;
}

void Buffer::destroyImpl() {
  if (allocation_ != VK_NULL_HANDLE) {
    vmaDestroyBuffer(std::get<VmaAllocator>(context_), handle_, allocation_);
    allocation_ = VK_NULL_HANDLE;
  }
}

void* Buffer::map() const {
  void* mappedMemory = nullptr;
  if (vmaMapMemory(std::get<VmaAllocator>(context_), allocation_, &mappedMemory) != VK_SUCCESS) {
    log().fatal("Failed to map buffer memory!");
  }
  return mappedMemory;
}

void Buffer::unmap() const {
  if (allocation_) {
    vmaUnmapMemory(std::get<VmaAllocator>(context_), allocation_);
  }
}

} // namespace aur