#include "DescriptorPool.h"

#include <volk/volk.h>

#include "../Logger.h"

#include <VkBootstrap.h>

namespace aur {
DescriptorPool::DescriptorPool(VkDevice device, const DescriptorPoolCreateInfo& poolCreateInfo)
    : device_{device}
    , createInfo_{poolCreateInfo}
    , handle_{[this, device, &poolCreateInfo]() {
      std::vector<VkDescriptorPoolSize> vkPoolSizes;
      vkPoolSizes.reserve(createInfo_.poolSizes.size());
      for (const auto& size : createInfo_.poolSizes) {
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

      VkDescriptorPool hnd{};
      VK(vkCreateDescriptorPool(device, &descPoolInfo, nullptr, &hnd));
      return hnd;
    }()} {}

DescriptorPool::~DescriptorPool() {
  destroy();
}

DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
    : device_{std::exchange(other.device_, {})}
    , createInfo_{std::exchange(other.createInfo_, {})}
    , handle_{std::exchange(other.handle_, {})} {}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept {
  if (this == &other)
    return *this;

  destroy();

  device_ = std::exchange(other.device_, {});
  createInfo_ = std::exchange(other.createInfo_, {});
  handle_ = std::exchange(other.handle_, {});
  return *this;
}

void DescriptorPool::invalidate() {
  handle_ = VK_NULL_HANDLE;
}

void DescriptorPool::destroy() {
  if (!isValid())
    return;

  vkDestroyDescriptorPool(device_, handle_, nullptr);
  invalidate();
}
} // namespace aur