#include "Allocator.h"
#include "../Logger.h"
#include "../Utils.h"
#include "../VulkanContext.h"

#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#include <VulkanMemoryAllocator/vk_mem_alloc.h>

namespace aur {
Allocator::Allocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
                     const AllocatorCreateInfo& allocatorCreateInfo)
    : createInfo(allocatorCreateInfo) {
  VmaVulkanFunctions vmaVulkanFunctions = {
      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
  };

  const VmaAllocatorCreateInfo vkCreateInfo = {
      .physicalDevice = physicalDevice,
      .device = device,
      .pVulkanFunctions = &vmaVulkanFunctions,
      .instance = instance,
      .vulkanApiVersion = createInfo.vulkanApiVersion,
  };

  VmaAllocator hnd;
  VK(vmaCreateAllocator(&vkCreateInfo, &hnd));
  const_cast<VmaAllocator&>(handle) = hnd;

  log().trace("VMA allocator created.");
}

Allocator::~Allocator() {
  destroy();
}

Allocator::Allocator(Allocator&& other) noexcept
    : handle(other.handle) {
  other.invalidate();
}

Allocator& Allocator::operator=(Allocator&& other) noexcept {
  if (this != &other) {
    destroy();

    const_cast<VmaAllocator&>(handle) = other.handle;
    other.invalidate();
  }
  return *this;
}
void Allocator::destroy() {
  if (isValid())
    vmaDestroyAllocator(handle);
  invalidate();
}

void Allocator::invalidate() {
  const_cast<VmaAllocator&>(handle) = VK_NULL_HANDLE;
}

} // namespace aur