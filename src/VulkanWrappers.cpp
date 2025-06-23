#include "VulkanWrappers.h"

#include <volk/volk.h>

namespace aur {
static_assert(static_cast<u32>(DescriptorType::UniformBuffer) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

static_assert(static_cast<u32>(ShaderStage::Vertex) == VK_SHADER_STAGE_VERTEX_BIT);
static_assert(static_cast<u32>(ShaderStage::Fragment) == VK_SHADER_STAGE_FRAGMENT_BIT);

static_assert(static_cast<u32>(VertexInputRate::Vertex) == VK_VERTEX_INPUT_RATE_VERTEX);
static_assert(static_cast<u32>(VertexInputRate::Instance) == VK_VERTEX_INPUT_RATE_INSTANCE);

static_assert(static_cast<u32>(Format::R32_SFLOAT) == VK_FORMAT_R32_SFLOAT);
static_assert(static_cast<u32>(Format::R32G32_SFLOAT) == VK_FORMAT_R32G32_SFLOAT);
static_assert(static_cast<u32>(Format::R32G32B32_SFLOAT) == VK_FORMAT_R32G32B32_SFLOAT);
static_assert(static_cast<u32>(Format::R32G32B32A32_SFLOAT) == VK_FORMAT_R32G32B32A32_SFLOAT);

template <typename TEnum>
u32 toVkFlags(const std::vector<TEnum>& enums) {
  u32 flags{};
  for (const auto& enm : enums)
    flags |= static_cast<u32>(enm);
  return flags;
}

template u32 toVkFlags(const std::vector<ShaderStage>& enums);

} // namespace aur