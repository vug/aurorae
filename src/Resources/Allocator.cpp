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
    : createInfo_(allocatorCreateInfo)
    , handle_([this, &instance, &physicalDevice, &device]() {
      VmaVulkanFunctions vmaVulkanFunctions = {
          .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
          .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
      };

      const VmaAllocatorCreateInfo vkCreateInfo = {
          .physicalDevice = physicalDevice,
          .device = device,
          .pVulkanFunctions = &vmaVulkanFunctions,
          .instance = instance,
          .vulkanApiVersion = createInfo_.vulkanApiVersion,
      };

      VmaAllocator hnd{};
      VK(vmaCreateAllocator(&vkCreateInfo, &hnd));
      return hnd;
    }()) {

  log().trace("VMA allocator created.");
}

Allocator::~Allocator() {
  destroy();
}

Allocator::Allocator(Allocator&& other) noexcept
    : createInfo_{std::exchange(other.createInfo_, {})}
    , handle_{std::exchange(other.handle_, {})} {}

Allocator& Allocator::operator=(Allocator&& other) noexcept {
  if (this == &other)
    return *this;

  destroy();
  createInfo_ = std::exchange(other.createInfo_, {});
  handle_ = std::exchange(other.handle_, {});
  return *this;
}
void Allocator::destroy() {
  if (!isValid())
    return;

  vmaDestroyAllocator(handle_);
  invalidate();
}

void Allocator::invalidate() {
  handle_ = VK_NULL_HANDLE;
}

} // namespace aur