#include "DescriptorSet.h"

#include "DescriptorSetLayout.h"
#include "Logger.h"

namespace aur {

#include <volk/volk.h>

DescriptorSet::DescriptorSet(VkDevice device, VkDescriptorPool pool,
                             const DescriptorSetCreateInfo& setCreateInfo)
    : createInfo(setCreateInfo)
    , handle([this, &device]() -> VkDescriptorSet {
      // Define the allocation information
      VkDescriptorSetAllocateInfo allocInfo{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool = pool_,
          .descriptorSetCount = 1,
          .pSetLayouts = &createInfo.layout.handle,
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

void DescriptorSet::invalidate() {
  const_cast<VkDescriptorSet&>(handle) = VK_NULL_HANDLE;
}

void DescriptorSet::destroy() {
  if (isValid()) {
    vkFreeDescriptorSets(device_, pool_, 1, &handle);
  }
}

DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept {}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept {}

// void DescriptorSet::update(const std::vector<VkWriteDescriptorSet>& writes) {
//   if (!isValid() || context_ == VK_NULL_HANDLE) {
//     aur::log().warn("Attempted to update an invalid or uninitialized DescriptorSet.");
//     return;
//   }
//
//   // A temporary vector to ensure `dstSet` points to *this* descriptor set.
//   // It's a common pattern to ensure the caller doesn't accidentally
//   // provide writes for a different set.
//   std::vector<VkWriteDescriptorSet> actualWrites = writes;
//   for (auto& write : actualWrites) {
//     write.dstSet = handle; // Ensure the write is applied to *this* descriptor set
//   }
//
//   // Perform the Vulkan API call to update the descriptor set.
//   // The last two parameters (dstSetCount, pDstSets) are for copying, not writing.
//   vkUpdateDescriptorSets(context_, static_cast<uint32_t>(actualWrites.size()), actualWrites.data(),
//                          0,        // No VkCopyDescriptorSet structures
//                          nullptr); // No VkCopyDescriptorSet structures
// }

// void DescriptorSet::destroy() {
//   // Check if the handle and context are valid before attempting to free.
//   // The pool_ must also be valid, as descriptor sets are freed from their pool.
//   if (isValid() && pool_ != VK_NULL_HANDLE && context_ != VK_NULL_HANDLE) {
//     // Free the descriptor set back to its pool.
//     // vkFreeDescriptorSets takes an array, so we pass a pointer to our single handle.
//     vkFreeDescriptorSets(context_, pool_, 1, &handle);
//   }
//   // Invalidate the base class's members (handle, createInfo) and our own pool_ reference.
//   invalidate_base_members();
//   pool_ = VK_NULL_HANDLE;
// }

} // namespace aur