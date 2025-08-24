#pragma once

#include "../VulkanWrappers.h"
#include "VulkanResource.h"

namespace aur {

class DescriptorSetLayout;
class DescriptorSet;
class Buffer;

struct DescriptorBufferInfo {
  const Buffer* buffer{};
  u64 offset{};
  u64 range{};
};

// typedef struct VkDescriptorImageInfo {
//   VkSampler        sampler;
//   VkImageView      imageView;
//   VkImageLayout    imageLayout;
// } VkDescriptorImageInfo;

struct WriteDescriptorSet {
  u32 binding{};
  u32 descriptorCnt{};
  DescriptorType descriptorType{};
  // DescriptorImageInfo* imageInfo{};
  DescriptorBufferInfo* bufferInfo{};
};

struct DescriptorSetCreateInfo {
  // This layout defines the structure of the descriptor set being allocated.
  // The DescriptorSet itself doesn't own the layout, just references it.
  const DescriptorSetLayout* layout;
};

class DescriptorSet : public VulkanResource<DescriptorSet, VkDescriptorSet, DescriptorSetCreateInfo, VkDevice,
                                            VkDescriptorPool> {
public:
  DescriptorSet() = default;
  DescriptorSet(VkDevice device, VkDescriptorPool pool, const DescriptorSetCreateInfo& createInfo);
  ~DescriptorSet();

  DescriptorSet(DescriptorSet&& other) noexcept = default;
  DescriptorSet& operator=(DescriptorSet&& other) noexcept = default;

  void update(const std::vector<WriteDescriptorSet>& writes) const;

private:
  friend class VulkanResource<DescriptorSet, VkDescriptorSet, DescriptorSetCreateInfo, VkDevice,
                              VkDescriptorPool>;

  // Static creation method for the handle
  static VkDescriptorSet createImpl(DescriptorSet* self, const DescriptorSetCreateInfo& createInfo,
                                    const std::tuple<VkDevice, VkDescriptorPool>& context);

  // Method to destroy the handle
  void destroyImpl() const;
};

} // namespace aur