#include "VulkanWrappers.h"

#include <volk/volk.h>

namespace aur {
static_assert(static_cast<u32>(DescriptorType::UniformBuffer) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

static_assert(static_cast<u32>(ShaderStage::Vertex) == VK_SHADER_STAGE_VERTEX_BIT);
static_assert(static_cast<u32>(ShaderStage::Fragment) == VK_SHADER_STAGE_FRAGMENT_BIT);

template <typename TEnum>
u32 toVkFlags(const std::vector<TEnum>& enums) {
  u32 flags{};
  for (const auto& enm : enums)
    flags |= static_cast<u32>(enm);
  return flags;
}

template u32 toVkFlags(const std::vector<ShaderStage>& enums);

} // namespace aur