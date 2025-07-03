#include "DescriptorSetLayout.h"

#include <volk/volk.h>

#include "../Logger.h"

namespace aur {

DescriptorSetLayout::DescriptorSetLayout(VkDevice device,
                                         const DescriptorSetLayoutCreateInfo& layoutCreateInfo)
    : device_{device}
    , createInfo_{layoutCreateInfo}
    , handle_{[this, device]() {
      std::vector<VkDescriptorSetLayoutBinding> vkBindings;
      for (const auto& binding : createInfo_.bindings) {

        vkBindings.push_back({
            .binding = binding.index,
            .descriptorType = static_cast<VkDescriptorType>(binding.type),
            .descriptorCount = binding.descriptorCount,
            .stageFlags = toVkFlags(binding.stages),
            .pImmutableSamplers = nullptr,
        });
      }

      VkDescriptorSetLayoutCreateInfo vkCreateInfo{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
          .bindingCount = static_cast<u32>(vkBindings.size()),
          .pBindings = vkBindings.data(),
      };

      VkDescriptorSetLayout hnd{};
      VK(vkCreateDescriptorSetLayout(device, &vkCreateInfo, nullptr, &hnd));
      return hnd;
    }()} {}

DescriptorSetLayout::~DescriptorSetLayout() {
  destroy();
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
    : device_{std::exchange(other.device_, {})}
    , createInfo_{std::exchange(other.createInfo_, {})}
    , handle_{std::exchange(other.handle_, {})} {}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept {
  if (this == &other)
    return *this;

  destroy();

  device_ = std::exchange(other.device_, {});
  createInfo_ = std::exchange(other.createInfo_, {});
  handle_ = std::exchange(other.handle_, {});

  return *this;
}
bool DescriptorSetLayout::isCompatible(const DescriptorSetLayout& other) const {
  const u32 bindingCnt = static_cast<u32>(createInfo_.bindings.size());
  if (bindingCnt != other.createInfo_.bindings.size())
    return false;

  for (u32 i = 0; i < bindingCnt; ++i) {
    const auto& thisBinding = createInfo_.bindings[i];
    const auto& otherBinding = other.createInfo_.bindings[i];
    if (thisBinding != otherBinding)
      return false;
  }
  return true;
}

void DescriptorSetLayout::invalidate() {
  handle_ = VK_NULL_HANDLE;
}
void DescriptorSetLayout::destroy() {
  if (!isValid())
    return;

  vkDestroyDescriptorSetLayout(device_, handle_, nullptr);
  invalidate();
}

} // namespace aur
