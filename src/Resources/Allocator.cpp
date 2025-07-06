#include "Allocator.h"

#include "../Logger.h"
#include "../Utils.h"
#include "../VulkanContext.h"

#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#include <VulkanMemoryAllocator/vk_mem_alloc.h>

namespace aur {

Allocator::Allocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
                     const AllocatorCreateInfo& createInfo)
    : VulkanResource(createInfo, instance, physicalDevice, device) {
  log().trace("VMA allocator created successfully.");
}

Allocator::~Allocator() {
  destroy();
}

VmaAllocator Allocator::createImpl([[maybe_unused]] Allocator* self, const AllocatorCreateInfo& createInfo,
                                   const std::tuple<VkInstance, VkPhysicalDevice, VkDevice>& context) {
  const auto& [instance, physicalDevice, device] = context; // Unpack the context tuple

  // Set up Vulkan function pointers required for VMA
  const VmaVulkanFunctions vmaVulkanFunctions = {
      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
  };

  // Configuration for VMA allocator
  const VmaAllocatorCreateInfo vkCreateInfo = {
      .physicalDevice = physicalDevice,
      .device = device,
      .pVulkanFunctions = &vmaVulkanFunctions,
      .instance = instance,
      .vulkanApiVersion = createInfo.vulkanApiVersion,
  };

  VmaAllocator hnd{VK_NULL_HANDLE};
  VK(vmaCreateAllocator(&vkCreateInfo, &hnd));
  return hnd;
}

void Allocator::destroyImpl() const {
  vmaDestroyAllocator(handle_);
}

} // namespace aur