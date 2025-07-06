#include "PipelineLayout.h"

#include "../Logger.h"
#include "DescriptorSetLayout.h"
#include <volk/volk.h>

namespace aur {

PipelineLayout::PipelineLayout(VkDevice device, const PipelineLayoutCreateInfo& createInfo)
    : VulkanResource{createInfo, device} {}

VkPipelineLayout PipelineLayout::createImpl([[maybe_unused]] PipelineLayout* self,
                                            const PipelineLayoutCreateInfo& createInfo,
                                            const std::tuple<VkDevice>& context) {
  std::vector<VkPushConstantRange> vkPushConstantRanges;
  for (const PushConstant& pc : createInfo.pushConstants) {
    u32 offset{};
    vkPushConstantRanges.push_back({
        .stageFlags = toVkFlags(pc.stages),
        .offset = offset,
        .size = pc.size,
    });
    offset += pc.size;
  }

  std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
  for (const DescriptorSetLayout* dsl : createInfo.descriptorSetLayouts) {
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
  VK(vkCreatePipelineLayout(std::get<0>(context), &vkCreateInfo, nullptr, &hnd));
  return hnd;
}

PipelineLayout::~PipelineLayout() {
  destroy();
}

void PipelineLayout::destroyImpl() const {
  vkDestroyPipelineLayout(std::get<VkDevice>(context_), handle_, nullptr);
}

} // namespace aur