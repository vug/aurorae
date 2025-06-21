#include "Allocator.h"
#include "../Logger.h"
#include "../Utils.h"
#include "../VulkanContext.h"

#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#include <VulkanMemoryAllocator/vk_mem_alloc.h>

namespace aur {
Allocator::Allocator(const aur::VulkanContext& context) {
  VmaVulkanFunctions vmaVulkanFunctions = {
      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
  };

  const VmaAllocatorCreateInfo createInfo = {
      .physicalDevice = context.getPhysicalDevice(),
      .device = context.getDevice(),
      .pVulkanFunctions = &vmaVulkanFunctions,
      .instance = context.getInstance(),
      .vulkanApiVersion = VK_API_VERSION_1_3,
  };

  VK(vmaCreateAllocator(&createInfo, &handle_));
  log().trace("VMA allocator created.");
}

Allocator::~Allocator() {
  if (handle_ != VK_NULL_HANDLE)
    vmaDestroyAllocator(handle_);
}

Allocator::Allocator(Allocator&& other) noexcept
    : handle_(other.handle_) {
  other.handle_ = VK_NULL_HANDLE;
}

Allocator& Allocator::operator=(Allocator&& other) noexcept {
  if (this != &other) {
    if (handle_ != VK_NULL_HANDLE) {
      vmaDestroyAllocator(handle_);
    }
    handle_ = other.handle_;
    other.handle_ = VK_NULL_HANDLE;
  }
  return *this;
}

} // namespace aur