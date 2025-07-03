#pragma once

#include "../VulkanWrappers.h"

namespace aur {

class DescriptorSetLayout;
class DescriptorSet;
class Buffer;

struct DescriptorBufferInfo {
  Buffer& buffer;
  u64 offset;
  u64 range;
};

// typedef struct VkDescriptorImageInfo {
//   VkSampler        sampler;
//   VkImageView      imageView;
//   VkImageLayout    imageLayout;
// } VkDescriptorImageInfo;

struct WriteDescriptorSet {
  DescriptorSet& dstSet;
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

class DescriptorSet {
public:
  DescriptorSet() = default;
  DescriptorSet(VkDevice device, VkDescriptorPool pool, const DescriptorSetCreateInfo& createInfo);
  ~DescriptorSet();

  DescriptorSet(const DescriptorSet&) = delete;
  DescriptorSet& operator=(const DescriptorSet&) = delete;
  DescriptorSet(DescriptorSet&& other) noexcept;
  DescriptorSet& operator=(DescriptorSet&& other) noexcept;

  [[nodiscard]] const DescriptorSetCreateInfo& getCreateInfo() const { return createInfo; }
  [[nodiscard]] const VkDescriptorSet& getHandle() const { return handle; }
  [[nodiscard]] inline bool isValid() const { return handle != VK_NULL_HANDLE; }

  const DescriptorSetCreateInfo createInfo{};
  const VkDescriptorSet handle{VK_NULL_HANDLE};

  // This method is called to actually bind resources to the descriptor set.
  void update(const std::vector<WriteDescriptorSet>& writes);

private:
  void invalidate();
  void destroy();

  VkDevice device_{VK_NULL_HANDLE};
  VkDescriptorPool pool_{VK_NULL_HANDLE};
};

} // namespace aur
