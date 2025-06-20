#include "DescriptorPool.h"

#include <volk/volk.h>

#include "Logger.h"

#include <VkBootstrap.h>

namespace aur {
DescriptorPool::DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo& poolCreateInfo)
    : createInfo(poolCreateInfo)
    , handle([this, device, &poolCreateInfo]() {
      std::vector<VkDescriptorPoolSize> vkPoolSizes;
      vkPoolSizes.reserve(createInfo.poolSizes.size());
      for (const auto& size : createInfo.poolSizes) {
        vkPoolSizes.push_back({
            .type = static_cast<VkDescriptorType>(size.type),
            .descriptorCount = size.count,
        });
      }
      const VkDescriptorPoolCreateInfo descPoolInfo{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
          .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
          .maxSets = 512,
          .poolSizeCount = static_cast<u32>(vkPoolSizes.size()),
          .pPoolSizes = vkPoolSizes.data(),
      };

      VkDescriptorPool hnd;
      VK(vkCreateDescriptorPool(device, &descPoolInfo, nullptr, &hnd));
      return hnd;
    }())
    , device_(device) {
  log().trace("Renderer descriptor pool created.");
}

DescriptorPool::~DescriptorPool() {
  destroy();
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
    : createInfo(std::move(other.createInfo))
    , handle(other.handle)
    , device_(other.device_) {
  other.invalidate();
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept {
  if (this != &other) {
    destroy();

    // Pilfer resources from other object
    const_cast<DescriptorPoolCreateInfo&>(createInfo) = std::move(other.createInfo);
    const_cast<VkDescriptorPool&>(handle) = other.handle;
    device_ = other.device_;

    other.invalidate();
  }
  return *this;
}

void DescriptorPool::invalidate() {
  const_cast<VkDescriptorPool&>(handle) = VK_NULL_HANDLE;
}
void DescriptorPool::destroy() {
  if (isValid() && device_ != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(device_, handle, nullptr);
}
} // namespace aur