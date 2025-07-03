#include "DescriptorSet.h"

#include "../Logger.h"
#include "Buffer.h"
#include "DescriptorSetLayout.h"

namespace aur {
#include <volk/volk.h>

DescriptorSet::DescriptorSet(VkDevice device, VkDescriptorPool pool,
                             const DescriptorSetCreateInfo& setCreateInfo)
    : device_{device}
    , pool_{pool}
    , createInfo_{setCreateInfo}
    , handle_{[this, device, pool]() -> VkDescriptorSet {
      VkDescriptorSetAllocateInfo allocInfo{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool = pool,
          .descriptorSetCount = 1,
          .pSetLayouts = &createInfo_.layout->getHandle(),
      };

      VkDescriptorSet hnd = VK_NULL_HANDLE;
      VK(vkAllocateDescriptorSets(device, &allocInfo, &hnd));
      return hnd;
    }()} {
  // At this point, the VkDescriptorSet handle is allocated, but the set
  // is conceptually "empty" or unpopulated. Resources will be bound via update().
}

DescriptorSet::~DescriptorSet() {
  destroy();
}

DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
    : device_{std::exchange(other.device_, {})}
    , pool_{std::exchange(other.pool_, {})}
    , createInfo_{std::exchange(other.createInfo_, {})}
    , handle_{std::exchange(other.handle_, {})} {}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept {
  if (this == &other)
    return *this;

  destroy();

  device_ = std::exchange(other.device_, {});
  pool_ = std::exchange(other.pool_, {});
  createInfo_ = std::exchange(other.createInfo_, {});
  handle_ = std::exchange(other.handle_, {});

  return *this;
}

void DescriptorSet::invalidate() {
  handle_ = VK_NULL_HANDLE;
}

void DescriptorSet::destroy() {
  if (!isValid())
    return;

  VK(vkFreeDescriptorSets(device_, pool_, 1, &handle_));

  invalidate();
}

void DescriptorSet::update(const std::vector<WriteDescriptorSet>& writes) const {
  // A temporary vector to ensure `dstSet` points to *this* descriptor set.
  // It's a common pattern to ensure the caller doesn't accidentally
  // provide writes for a different set.
  std::vector<VkDescriptorBufferInfo> vkBufferInfos;
  std::vector<VkWriteDescriptorSet> vkWrites;
  for (const auto& write : writes) {
    const DescriptorBufferInfo& bufferInfo = *write.bufferInfo;
    const VkDescriptorBufferInfo& vkBufferInfo =
        vkBufferInfos.emplace_back(bufferInfo.buffer.getHandle(), bufferInfo.offset, bufferInfo.range);
    vkWrites.push_back({.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = write.dstSet.handle_,
                        .dstBinding = write.binding,
                        .descriptorCount = write.descriptorCnt,
                        .descriptorType = static_cast<VkDescriptorType>(write.descriptorType),
                        .pBufferInfo = &vkBufferInfo});
  }

  // Perform the Vulkan API call to update the descriptor set.
  // The last two parameters (dstSetCount, pDstSets) are for copying, not writing.
  vkUpdateDescriptorSets(device_, static_cast<uint32_t>(vkWrites.size()), vkWrites.data(),
                         0,        // No VkCopyDescriptorSet structures
                         nullptr); // No VkCopyDescriptorSet structures
}

} // namespace aur