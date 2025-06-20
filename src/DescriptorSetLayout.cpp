#include "DescriptorSetLayout.h"

#include <volk/volk.h>

#include "Logger.h"

namespace aur {

DescriptorSetLayout::DescriptorSetLayout(VkDevice device,
                                         const DescriptorSetLayoutCreateInfo& layoutCreateInfo)
    : createInfo(layoutCreateInfo)
    , handle([this, device]() {
      std::vector<VkDescriptorSetLayoutBinding> vkBindings;
      for (const auto& binding : createInfo.bindings) {

        vkBindings.push_back({
            .binding = binding.index,
            .descriptorType = static_cast<VkDescriptorType>(binding.type),
            .descriptorCount = binding.descriptorCount,
            .stageFlags = toStageFlags(binding.stages),
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
  destroy();
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
    : createInfo(std::move(other.createInfo))
    , handle(other.handle)
    , device_(other.device_) {
  other.invalidate();
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept {
  if (this != &other) {
    destroy();

    // Pilfer resources from other object
    const_cast<DescriptorSetLayoutCreateInfo&>(createInfo) = std::move(other.createInfo);
    const_cast<VkDescriptorSetLayout&>(handle) = other.handle;
    device_ = other.device_;

    // Invalidate the other object
    other.invalidate();
  }
  return *this;
}
bool DescriptorSetLayout::isCompatible(const DescriptorSetLayout& other) const {
  const u32 bindingCnt = static_cast<u32>(createInfo.bindings.size());
  if (bindingCnt != other.createInfo.bindings.size())
    return false;

  for (u32 i = 0; i < bindingCnt; ++i) {
    const auto& thisBinding = createInfo.bindings[i];
    const auto& otherBinding = other.createInfo.bindings[i];
    if (thisBinding != otherBinding)
      return false;
  }
  return true;
}

void DescriptorSetLayout::invalidate() {
  const_cast<VkDescriptorSetLayout&>(handle) = VK_NULL_HANDLE;
  // not needed because device_ is not owned by DescriptorSetLayout
  device_ = VK_NULL_HANDLE;
}
void DescriptorSetLayout::destroy() {
  if (isValid() && device_ != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(device_, handle, nullptr);
  invalidate();
}

} // namespace aur
