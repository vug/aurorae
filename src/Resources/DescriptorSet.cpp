#include "DescriptorSet.h"
#include "../Logger.h"
#include "Buffer.h"
#include "DescriptorSetLayout.h"

#include <volk/volk.h>

namespace aur {

DescriptorSet::DescriptorSet(VkDevice device, VkDescriptorPool pool,
                             const DescriptorSetCreateInfo& createInfo)
    : VulkanResource{createInfo, device, pool} {}

DescriptorSet::~DescriptorSet() {
  destroy();
}

VkDescriptorSet DescriptorSet::createImpl([[maybe_unused]] DescriptorSet* self,
                                          const DescriptorSetCreateInfo& createInfo,
                                          const std::tuple<VkDevice, VkDescriptorPool>& context) {
  const VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = std::get<VkDescriptorPool>(context),
      .descriptorSetCount = 1,
      .pSetLayouts = &createInfo.layout->getHandle(),
  };

  VkDescriptorSet hnd = VK_NULL_HANDLE;
  VK(vkAllocateDescriptorSets(std::get<VkDevice>(context), &allocInfo, &hnd));
  // At this point, the VkDescriptorSet handle is allocated, but the set
  // is conceptually "empty" or unpopulated. Resources will be bound via update().
  return hnd;
}

void DescriptorSet::destroyImpl() const {
  vkFreeDescriptorSets(std::get<VkDevice>(context_), std::get<VkDescriptorPool>(context_), 1, &handle_);
}

void DescriptorSet::update(const std::vector<WriteDescriptorSet>& writes) const {
  const std::vector<DescriptorSetLayoutBinding>& setLayoutBindings =
      getCreateInfo().layout->getCreateInfo().bindings;
  for (const auto& [binding, write] : std::ranges::views::zip(setLayoutBindings, writes)) {
    if (binding.index != write.binding)
      log().fatal("Binding index {} does not match write binding no {} in DescriptorSet::update().",
                  binding.index, write.binding);
    if (binding.type != write.descriptorType)
      log().fatal("Binding type {} does not match write descriptor type {} in DescriptorSet::update().",
                  glz::write<glz::opts{.raw = true}>(binding.type).value_or("ERROR"),
                  glz::write<glz::opts{.raw = true}>(write.binding).value_or("ERROR"));
  }

  std::vector<VkDescriptorBufferInfo> vkBufferInfos;
  std::vector<VkWriteDescriptorSet> vkWrites;
  for (const auto& write : writes) {
    const DescriptorBufferInfo& bufferInfo = *write.bufferInfo;
    vkBufferInfos.push_back(
        {.buffer = bufferInfo.buffer->getHandle(), .offset = bufferInfo.offset, .range = bufferInfo.range});
    vkWrites.push_back({
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = getHandle(),
        .dstBinding = write.binding,
        .descriptorCount = write.descriptorCnt,
        .descriptorType = static_cast<VkDescriptorType>(write.descriptorType),
        .pBufferInfo = &vkBufferInfos.back(),
    });
  }

  // Perform the Vulkan API call to update the descriptor set.
  // The last two parameters (dstSetCount, pDstSets) are for copying, not writing.
  vkUpdateDescriptorSets(std::get<VkDevice>(context_), static_cast<u32>(vkWrites.size()), vkWrites.data(), 0,
                         nullptr);
}

} // namespace aur