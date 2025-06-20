#include "PipelineLayout.h"

#include "DescriptorSetLayout.h"
#include "Logger.h"
#include <volk/volk.h>

namespace aur {

PipelineLayout::PipelineLayout(VkDevice device, const PipelineLayoutCreateInfo& layoutCreateInfo)
    : createInfo(layoutCreateInfo)
    , handle([this, device]() {
      std::vector<VkPushConstantRange> vkPushConstantRanges;
      for (const PushConstant& pc : createInfo.pushConstants) {
        u32 offset{};
        vkPushConstantRanges.push_back({
            .stageFlags = toStageFlags(pc.stages),
            .offset = offset,
            .size = pc.size,
        });
        offset += pc.size;
      }

      std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
      for (const DescriptorSetLayout* dsl : createInfo.descriptorSetLayouts) {
        if (!dsl)
          log().fatal("DescriptorSetLayout* given to PipelineLayoutCreateInfo is null.");
        vkDescriptorSetLayouts.push_back(dsl->handle);
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
    }())
    , device_(device) {}

PipelineLayout::~PipelineLayout() {
  destroy();
}

PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
    : createInfo(std::move(other.createInfo))
    , handle(other.handle)
    , device_(other.device_) {
  other.invalidate();
}

PipelineLayout& PipelineLayout::operator=(PipelineLayout&& other) noexcept {
  if (this != &other) {
    destroy();

    // Pilfer resources from other object
    const_cast<PipelineLayoutCreateInfo&>(createInfo) = std::move(other.createInfo);
    const_cast<VkPipelineLayout&>(handle) = other.handle;
    device_ = other.device_;

    // Invalidate the other object
    other.invalidate();
  }
  return *this;
}

void PipelineLayout::invalidate() {
  const_cast<VkPipelineLayout&>(handle) = VK_NULL_HANDLE;
  // device_ is not owned by PipelineLayout, so it's only set to VK_NULL_HANDLE
  // to reflect that this object no longer uses a device, not to indicate destruction
  device_ = VK_NULL_HANDLE;
}

void PipelineLayout::destroy() {
  if (isValid() && device_ != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(device_, handle, nullptr);
  invalidate();
}

} // namespace aur