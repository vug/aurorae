#include "VulkanWrappers.h"

#include <VulkanMemoryAllocator/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace aur {
template <typename TEnum>
u32 toVkFlags(const std::vector<TEnum>& enums) {
  u32 flags{};
  for (const auto& enm : enums)
    flags |= static_cast<u32>(enm);
  return flags;
}

template u32 toVkFlags(const std::vector<ShaderStageType>& enums);
template u32 toVkFlags(const std::vector<BufferUsage>& enums);
template u32 toVkFlags(const std::vector<ColorComponent>& enums);

// clang-format off
static_assert(static_cast<u32>(DescriptorType::UniformBuffer) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

static_assert(static_cast<u32>(ShaderStageType::Vertex) == VK_SHADER_STAGE_VERTEX_BIT);
static_assert(static_cast<u32>(ShaderStageType::Fragment) == VK_SHADER_STAGE_FRAGMENT_BIT);

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

static_assert(static_cast<u32>(CullMode::None) == VK_CULL_MODE_NONE);
static_assert(static_cast<u32>(CullMode::Front) == VK_CULL_MODE_FRONT_BIT);
static_assert(static_cast<u32>(CullMode::Back) == VK_CULL_MODE_BACK_BIT);
static_assert(static_cast<u32>(CullMode::FrontAndBack) == VK_CULL_MODE_FRONT_AND_BACK);

static_assert(static_cast<u32>(PolygonMode::Fill) == VK_POLYGON_MODE_FILL);
static_assert(static_cast<u32>(PolygonMode::Line) == VK_POLYGON_MODE_LINE);
static_assert(static_cast<u32>(PolygonMode::Point) == VK_POLYGON_MODE_POINT);

static_assert(static_cast<u32>(FrontFace::CounterClockwise) == VK_FRONT_FACE_COUNTER_CLOCKWISE);
static_assert(static_cast<u32>(FrontFace::Clockwise) == VK_FRONT_FACE_CLOCKWISE);

static_assert(static_cast<u32>(BlendFactor::Zero) == VK_BLEND_FACTOR_ZERO);
static_assert(static_cast<u32>(BlendFactor::One) == VK_BLEND_FACTOR_ONE);
static_assert(static_cast<u32>(BlendFactor::SrcColor) == VK_BLEND_FACTOR_SRC_COLOR);
static_assert(static_cast<u32>(BlendFactor::OneMinusSrcColor) == VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
static_assert(static_cast<u32>(BlendFactor::DstColor) == VK_BLEND_FACTOR_DST_COLOR);
static_assert(static_cast<u32>(BlendFactor::OneMinusDstColor) == VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR);
static_assert(static_cast<u32>(BlendFactor::SrcAlpha) == VK_BLEND_FACTOR_SRC_ALPHA);
static_assert(static_cast<u32>(BlendFactor::OneMinusSrcAlpha) == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
static_assert(static_cast<u32>(BlendFactor::DstAlpha) == VK_BLEND_FACTOR_DST_ALPHA);
static_assert(static_cast<u32>(BlendFactor::OneMinusDstAlpha) == VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
static_assert(static_cast<u32>(BlendFactor::ConstantColor) == VK_BLEND_FACTOR_CONSTANT_COLOR);
static_assert(static_cast<u32>(BlendFactor::OneMinusConstantColor) ==  VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR);
static_assert(static_cast<u32>(BlendFactor::ConstantAlpha) == VK_BLEND_FACTOR_CONSTANT_ALPHA);
static_assert(static_cast<u32>(BlendFactor::OneMinusConstantAlpha) == VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA);
static_assert(static_cast<u32>(BlendFactor::SrcAlphaSaturate) == VK_BLEND_FACTOR_SRC_ALPHA_SATURATE);
static_assert(static_cast<u32>(BlendFactor::Src1Color) == VK_BLEND_FACTOR_SRC1_COLOR);
static_assert(static_cast<u32>(BlendFactor::OneMinusSrc1Color) == VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR);
static_assert(static_cast<u32>(BlendFactor::Src1Alpha) == VK_BLEND_FACTOR_SRC1_ALPHA);
static_assert(static_cast<u32>(BlendFactor::OneMinusSrc1Alpha) == VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA);

static_assert(static_cast<u32>(BlendOp::Add) == VK_BLEND_OP_ADD);
static_assert(static_cast<u32>(BlendOp::Subtract) == VK_BLEND_OP_SUBTRACT);
static_assert(static_cast<u32>(BlendOp::ReverseSubtract) == VK_BLEND_OP_REVERSE_SUBTRACT);
static_assert(static_cast<u32>(BlendOp::Min) == VK_BLEND_OP_MIN);
static_assert(static_cast<u32>(BlendOp::Max) == VK_BLEND_OP_MAX);

static_assert(static_cast<u32>(ColorComponent::Red) == VK_COLOR_COMPONENT_R_BIT);
static_assert(static_cast<u32>(ColorComponent::Green) == VK_COLOR_COMPONENT_G_BIT);
static_assert(static_cast<u32>(ColorComponent::Blue) == VK_COLOR_COMPONENT_B_BIT);
static_assert(static_cast<u32>(ColorComponent::Alpha) == VK_COLOR_COMPONENT_A_BIT);

static_assert(static_cast<u32>(LogicOp::Clear) == VK_LOGIC_OP_CLEAR);
static_assert(static_cast<u32>(LogicOp::And) == VK_LOGIC_OP_AND);
static_assert(static_cast<u32>(LogicOp::AndReverse) == VK_LOGIC_OP_AND_REVERSE);
static_assert(static_cast<u32>(LogicOp::Copy) == VK_LOGIC_OP_COPY);
static_assert(static_cast<u32>(LogicOp::AndInverted) == VK_LOGIC_OP_AND_INVERTED);
static_assert(static_cast<u32>(LogicOp::NoOp) == VK_LOGIC_OP_NO_OP);
static_assert(static_cast<u32>(LogicOp::Xor) == VK_LOGIC_OP_XOR);
static_assert(static_cast<u32>(LogicOp::Or) == VK_LOGIC_OP_OR);
static_assert(static_cast<u32>(LogicOp::Nor) == VK_LOGIC_OP_NOR);
static_assert(static_cast<u32>(LogicOp::Equivalent) == VK_LOGIC_OP_EQUIVALENT);
static_assert(static_cast<u32>(LogicOp::Invert) == VK_LOGIC_OP_INVERT);
static_assert(static_cast<u32>(LogicOp::OrReverse) == VK_LOGIC_OP_OR_REVERSE);
static_assert(static_cast<u32>(LogicOp::CopyInverted) == VK_LOGIC_OP_COPY_INVERTED);
static_assert(static_cast<u32>(LogicOp::OrInverted) == VK_LOGIC_OP_OR_INVERTED);
static_assert(static_cast<u32>(LogicOp::Nand) == VK_LOGIC_OP_NAND);
static_assert(static_cast<u32>(LogicOp::Set) == VK_LOGIC_OP_SET);
// clang-format on
} // namespace aur