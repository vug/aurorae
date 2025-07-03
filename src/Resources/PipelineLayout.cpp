#include "PipelineLayout.h"

#include "../Logger.h"
#include "DescriptorSetLayout.h"
#include <volk/volk.h>

namespace aur {

PipelineLayout::PipelineLayout(VkDevice device, const PipelineLayoutCreateInfo& layoutCreateInfo)
    : device_{device}
    , createInfo_{layoutCreateInfo}
    , handle_{[this, device]() {
      std::vector<VkPushConstantRange> vkPushConstantRanges;
      for (const PushConstant& pc : createInfo_.pushConstants) {
        u32 offset{};
        vkPushConstantRanges.push_back({
            .stageFlags = toVkFlags(pc.stages),
            .offset = offset,
            .size = pc.size,
        });
        offset += pc.size;
      }

      std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
      for (const DescriptorSetLayout* dsl : createInfo_.descriptorSetLayouts) {
        if (!dsl)
          log().fatal("DescriptorSetLayout* given to PipelineLayoutCreateInfo is null.");
        vkDescriptorSetLayouts.push_back(dsl->getHandle());
      }
      const VkPipelineLayoutCreateInfo vkCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
          .setLayoutCount = static_cast<u32>(vkDescriptorSetLayouts.size()),
          .pSetLayouts = vkDescriptorSetLayouts.data(),
          .pushConstantRangeCount = static_cast<u32>(vkPushConstantRanges.size()),
          .pPushConstantRanges = vkPushConstantRanges.data(),
      };

      VkPipelineLayout hnd{VK_NULL_HANDLE};
      VK(vkCreatePipelineLayout(device, &vkCreateInfo, nullptr, &hnd));
      return hnd;
    }()} {}

PipelineLayout::~PipelineLayout() {
  destroy();
}

PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
    : device_{std::exchange(other.device_, {})}
    , createInfo_{std::exchange(other.createInfo_, {})}
    , handle_{std::exchange(other.handle_, {})} {}

PipelineLayout& PipelineLayout::operator=(PipelineLayout&& other) noexcept {
  if (this == &other)
    return *this;

  destroy();

  device_ = std::exchange(other.device_, {});
  createInfo_ = std::exchange(other.createInfo_, {});
  handle_ = std::exchange(other.handle_, {});

  return *this;
}

void PipelineLayout::invalidate() {
  handle_ = VK_NULL_HANDLE;
}

void PipelineLayout::destroy() {
  if (!isValid())
    return;

  vkDestroyPipelineLayout(device_, handle_, nullptr);
  invalidate();
}

} // namespace aur