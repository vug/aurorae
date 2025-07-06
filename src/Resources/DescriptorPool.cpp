#include "DescriptorPool.h"

#include <volk/volk.h>

#include "../Logger.h"

namespace aur {

DescriptorPool::DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo& createInfo)
    : VulkanResource{createInfo, device} {}

DescriptorPool::~DescriptorPool() {
  destroy();
}

VkDescriptorPool DescriptorPool::createImpl(const DescriptorPoolCreateInfo& createInfo,
                                            const std::tuple<VkDevice>& context) {
  std::vector<VkDescriptorPoolSize> vkPoolSizes;
  vkPoolSizes.reserve(createInfo.poolSizes.size());
  for (const auto& size : createInfo.poolSizes) {
    vkPoolSizes.push_back({
        .type = static_cast<VkDescriptorType>(size.type),
        .descriptorCount = size.count,
    });
  }

  const VkDescriptorPoolCreateInfo vkCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = createInfo.maxSets,
      .poolSizeCount = static_cast<u32>(vkPoolSizes.size()),
      .pPoolSizes = vkPoolSizes.data(),
  };

  VkDescriptorPool hnd{VK_NULL_HANDLE};
  VK(vkCreateDescriptorPool(std::get<VkDevice>(context), &vkCreateInfo, nullptr, &hnd));

  return hnd;
}

void DescriptorPool::destroyImpl() const {
  vkDestroyDescriptorPool(std::get<VkDevice>(context_), handle_, nullptr);
}

} // namespace aur