#include "DescriptorSetLayout.h"

#include "../Logger.h"
#include <volk/volk.h>

namespace aur {

DescriptorSetLayout::DescriptorSetLayout(VkDevice device, const DescriptorSetLayoutCreateInfo& createInfo)
    : VulkanResource{createInfo, device} {}

DescriptorSetLayout::~DescriptorSetLayout() {
  destroy();
}

VkDescriptorSetLayout DescriptorSetLayout::createImpl([[maybe_unused]] DescriptorSetLayout* self,
                                                      const DescriptorSetLayoutCreateInfo& createInfo,
                                                      const std::tuple<VkDevice>& context) {
  std::vector<VkDescriptorSetLayoutBinding> vkBindings;
  for (const auto& binding : createInfo.bindings) {
    vkBindings.push_back({
        .binding = binding.index,
        .descriptorType = static_cast<VkDescriptorType>(binding.type),
        .descriptorCount = binding.descriptorCount,
        .stageFlags = toVkFlags(binding.stages),
        .pImmutableSamplers = nullptr,
    });
  }

  const VkDescriptorSetLayoutCreateInfo vkCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<u32>(vkBindings.size()),
      .pBindings = vkBindings.data(),
  };

  VkDescriptorSetLayout hnd{VK_NULL_HANDLE};
  VK(vkCreateDescriptorSetLayout(std::get<VkDevice>(context), &vkCreateInfo, nullptr, &hnd));
  return hnd;
}

void DescriptorSetLayout::destroyImpl() const {
  vkDestroyDescriptorSetLayout(std::get<VkDevice>(context_), handle_, nullptr);
}

bool DescriptorSetLayout::isCompatible(const DescriptorSetLayout& other) const {
  const auto& bindings1 = getCreateInfo().bindings;
  const auto& bindings2 = other.getCreateInfo().bindings;

  if (bindings1.size() != bindings2.size()) {
    return false;
  }

  for (size_t i = 0; i < bindings1.size(); ++i) {
    if (bindings1[i] != bindings2[i]) {
      return false;
    }
  }

  return true;
}

} // namespace aur