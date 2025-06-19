#include "DescriptorSetLayout.h"

#include <bitset>

#include <volk/volk.h>

#include "Logger.h"

namespace aur {

DescriptorSetLayout::DescriptorSetLayout(VkDevice device,
                                         const DescriptorSetLayoutCreateInfo& layoutCreateInfo)
    : createInfo(layoutCreateInfo)
    , handle([this, device]() {
      std::vector<VkDescriptorSetLayoutBinding> vkBindings;
      for (const auto& binding : createInfo.bindings) {
        std::bitset<32> stageFlags;
        for (const auto& stage : binding.stages)
          stageFlags |= std::bitset<32>(static_cast<u32>(stage));
        vkBindings.push_back({
            .binding = binding.index,
            .descriptorType = static_cast<VkDescriptorType>(binding.type),
            .descriptorCount = binding.descriptorCount,
            .stageFlags = static_cast<u32>(stageFlags.to_ulong()),
            .pImmutableSamplers = nullptr,
        });
      }

      VkDescriptorSetLayoutCreateInfo vkCreateInfo{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
          .bindingCount = static_cast<u32>(vkBindings.size()),
          .pBindings = vkBindings.data(),
      };

      VkDescriptorSetLayout hnd{VK_NULL_HANDLE};
      VK(vkCreateDescriptorSetLayout(device, &vkCreateInfo, nullptr, &hnd));
      return hnd;
    }())
    , device_(device) {}

DescriptorSetLayout::~DescriptorSetLayout() {
  if (!isValid())
    return;
  vkDestroyDescriptorSetLayout(device_, handle, nullptr);
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
    : createInfo(std::move(other.createInfo))
    , handle(other.handle)
    , device_(other.device_) {
  // Invalidate the other object so its destructor doesn't destroy the handle
  const_cast<VkDescriptorSetLayout&>(other.handle) = VK_NULL_HANDLE;
  other.device_ = VK_NULL_HANDLE;
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept {
  if (this != &other) {
    // Destroy the existing resource before taking ownership of the new one
    if (device_ != VK_NULL_HANDLE && handle != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(device_, handle, nullptr);
    }

    // Pilfer resources from other object
    const_cast<DescriptorSetLayoutCreateInfo&>(createInfo) = std::move(other.createInfo);
    const_cast<VkDescriptorSetLayout&>(handle) = other.handle;
    device_ = other.device_;

    // Invalidate the other object
    const_cast<VkDescriptorSetLayout&>(other.handle) = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
  }
  return *this;
}

} // namespace aur
