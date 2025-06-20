#include "VulkanWrappers.h"

#include <bitset>

#include <volk/volk.h>

namespace aur {
static_assert(static_cast<u32>(DescriptorType::UniformBuffer) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

static_assert(static_cast<u32>(ShaderStage::Vertex) == VK_SHADER_STAGE_VERTEX_BIT);
static_assert(static_cast<u32>(ShaderStage::Fragment) == VK_SHADER_STAGE_FRAGMENT_BIT);

u32 toStageFlags(const std::vector<ShaderStage>& stages) {
  std::bitset<32> stageFlags;
  for (const auto& stage : stages)
    stageFlags |= std::bitset<32>(static_cast<u32>(stage));
  return static_cast<u32>(stageFlags.to_ulong());
}
} // namespace aur