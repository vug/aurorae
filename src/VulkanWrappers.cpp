#include "VulkanWrappers.h"

// clang-format off
#include <volk/volk.h>
#include <VulkanMemoryAllocator/vk_mem_alloc.h>
// clang-format on

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

static_assert(static_cast<u32>(BufferUsage::TransferSrc) == VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
static_assert(static_cast<u32>(BufferUsage::TransferDst) == VK_BUFFER_USAGE_TRANSFER_DST_BIT);
static_assert(static_cast<u32>(BufferUsage::Uniform) == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
static_assert(static_cast<u32>(BufferUsage::Index) == VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
static_assert(static_cast<u32>(BufferUsage::Vertex) == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

static_assert(static_cast<u32>(MemoryUsage::Unknown) == VMA_MEMORY_USAGE_UNKNOWN);
static_assert(static_cast<u32>(MemoryUsage::GpuOnly) == VMA_MEMORY_USAGE_GPU_ONLY);
static_assert(static_cast<u32>(MemoryUsage::CpuOnly) == VMA_MEMORY_USAGE_CPU_ONLY);
static_assert(static_cast<u32>(MemoryUsage::CpuToGpu) == VMA_MEMORY_USAGE_CPU_TO_GPU);
static_assert(static_cast<u32>(MemoryUsage::GpuToCpu) == VMA_MEMORY_USAGE_GPU_TO_CPU);

template <typename TEnum>
u32 toVkFlags(const std::vector<TEnum>& enums) {
  u32 flags{};
  for (const auto& enm : enums)
    flags |= static_cast<u32>(enm);
  return flags;
}

template u32 toVkFlags(const std::vector<ShaderStage>& enums);
template u32 toVkFlags(const std::vector<BufferUsage>& enums);

} // namespace aur