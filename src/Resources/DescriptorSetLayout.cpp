#include "DescriptorSetLayout.h"

#include "../Logger.h"
#include <volk/volk.h>

#include <algorithm>
#include <set>

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
  namespace rng = std::ranges;
  namespace vws = std::ranges::views;

  const auto thisBindingMap =
      getCreateInfo().bindings |
      vws::transform([](const auto& binding) { return std::make_pair(binding.index, &binding); }) |
      rng::to<std::unordered_map>();
  const auto otherBindingMap =
      other.getCreateInfo().bindings |
      vws::transform([](const auto& binding) { return std::make_pair(binding.index, &binding); }) |
      rng::to<std::unordered_map>();

  // if other is not superset not compatible
  if (!rng::includes(otherBindingMap | vws::keys | rng::to<std::set>(),
                     thisBindingMap | vws::keys | rng::to<std::set>()))
    return false;

  for (const auto& [bindingIndex, thisBinding] : thisBindingMap) {
    const auto* otherBinding = otherBindingMap.at(bindingIndex);
    // Check core compatibility: type and count must match exactly
    if (thisBinding->type != otherBinding->type ||
        thisBinding->descriptorCount != otherBinding->descriptorCount)
      return false;

    // Stage flags can be still compatible even if different as long as other bindings stages is a superset
    if (!rng::includes(otherBinding->stages, thisBinding->stages))
      return false;
  }

  return true;
}

} // namespace aur