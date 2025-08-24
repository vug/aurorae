#include "Buffer.h"

#include "../Logger.h"
#include <VulkanMemoryAllocator/vk_mem_alloc.h>
#include <cassert>

namespace aur {

Buffer::Buffer(VmaAllocator allocator, const BufferCreateInfo& createInfo)
    : VulkanResource(createInfo, allocator) {
  // The VulkanResource base class automatically calls `createImpl` during initialization,
  // which assigns the allocation_.
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

  VkBuffer hnd = VK_NULL_HANDLE;
  VK(vmaCreateBuffer(std::get<VmaAllocator>(context), &bufferInfo, &allocInfo, &hnd, &self->allocation_,
                     nullptr));

  return hnd;
}

void Buffer::destroyImpl() {
  if (allocation_ != VK_NULL_HANDLE) {
    vmaDestroyBuffer(std::get<VmaAllocator>(context_), handle_, allocation_);
    allocation_ = VK_NULL_HANDLE;
  }
}

void Buffer::map() {
  void* mappedMemory = nullptr;
  VK(vmaMapMemory(std::get<VmaAllocator>(context_), allocation_, &mappedMemory));
  mapPtr_ = static_cast<std::byte*>(mappedMemory);
}

std::byte* Buffer::getMapPtr() const {
  if (!mapPtr_)
    log().fatal("Buffer::mapPtr() called before map()!");
  return mapPtr_;
}

void Buffer::unmap() const {
  if (allocation_) {
    vmaUnmapMemory(std::get<VmaAllocator>(context_), allocation_);
  }
}

} // namespace aur