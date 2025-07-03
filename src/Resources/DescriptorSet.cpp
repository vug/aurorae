#include "DescriptorSet.h"

#include "../Logger.h"
#include "Buffer.h"
#include "DescriptorSetLayout.h"

namespace aur {

#include <volk/volk.h>

DescriptorSet::DescriptorSet(VkDevice device, VkDescriptorPool pool,
                             const DescriptorSetCreateInfo& setCreateInfo)
    : createInfo(setCreateInfo)
    , handle([this, device, pool]() -> VkDescriptorSet {
      VkDescriptorSetAllocateInfo allocInfo{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool = pool,
          .descriptorSetCount = 1,
          .pSetLayouts = &createInfo.layout->handle,
      };

      VkDescriptorSet dset = VK_NULL_HANDLE;
      VK(vkAllocateDescriptorSets(device, &allocInfo, &dset));
      return dset;
    }())
    , pool_(pool)
    , device_(device) {
  // At this point, the VkDescriptorSet handle is allocated, but the set
  // is conceptually "empty" or unpopulated. Resources will be bound via update().
}

DescriptorSet::~DescriptorSet() {
  destroy();
}

DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
    : createInfo(std::move(other.createInfo))
    , handle(other.handle)
    , device_(other.device_)
    , pool_(other.pool_) {
  other.invalidate();
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept {
  if (this != &other) {
    destroy();

    // Pilfer resources from other object
    const_cast<DescriptorSetCreateInfo&>(createInfo) = std::move(other.createInfo);
    const_cast<VkDescriptorSet&>(handle) = other.handle;
    device_ = other.device_;
    pool_ = other.pool_;

    other.invalidate();
  }
  return *this;
}

void DescriptorSet::invalidate() {
  const_cast<VkDescriptorSet&>(handle) = VK_NULL_HANDLE;
}

void DescriptorSet::destroy() {
  if (isValid() && device_ != VK_NULL_HANDLE && pool_ != VK_NULL_HANDLE)
    VK(vkFreeDescriptorSets(device_, pool_, 1, &handle));
}

void DescriptorSet::update(const std::vector<WriteDescriptorSet>& writes) {
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
                        .dstSet = write.dstSet.handle,
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