#pragma once

#include <vector>

#include "Utils.h"

#define FORWARD_DEFINE_VK_HANDLE(object) typedef struct object##_T* object;
#if !defined(VK_NULL_HANDLE)
#define VK_NULL_HANDLE nullptr
#endif

// Forward definitions of vulkan handles and types so that we don't have to include vulkan headers in Aurorae
// headers

FORWARD_DEFINE_VK_HANDLE(VkInstance)
FORWARD_DEFINE_VK_HANDLE(VkSurfaceKHR)
FORWARD_DEFINE_VK_HANDLE(VkPhysicalDevice)
FORWARD_DEFINE_VK_HANDLE(VkDevice)
FORWARD_DEFINE_VK_HANDLE(VmaAllocator)
FORWARD_DEFINE_VK_HANDLE(VmaAllocation)
FORWARD_DEFINE_VK_HANDLE(VkBuffer)
FORWARD_DEFINE_VK_HANDLE(VkDescriptorPool)
FORWARD_DEFINE_VK_HANDLE(VkDescriptorSetLayout)
FORWARD_DEFINE_VK_HANDLE(VkDescriptorSet)
FORWARD_DEFINE_VK_HANDLE(VkShaderModule)
FORWARD_DEFINE_VK_HANDLE(VkPipelineLayout)
FORWARD_DEFINE_VK_HANDLE(VkPipelineCache)
FORWARD_DEFINE_VK_HANDLE(VkPipeline)
using VkDeviceSize = uint64_t;
using VkBufferUsageFlags = uint32_t;

namespace aur {
// sync with VkDescriptorType
enum class DescriptorType : u32 {
  UniformBuffer = 6,
};

// sync with VkShaderStageFlagBits
enum class ShaderStageType : u32 {
  Vertex = 0x00000001,
  Fragment = 0x00000010,
};

// sync with VkVertexInputRate
enum class VertexInputRate : u32 {
  Vertex = 0,
  Instance = 1,
};

// sync with VkFormat
enum class Format : u32 {
  R32_SFLOAT = 100,
  R32G32_SFLOAT = 103,
  R32G32B32_SFLOAT = 106,
  R32G32B32A32_SFLOAT = 109,
};

// sync with VkBufferUsageFlagBits
enum class BufferUsage {
  TransferSrc = 0x00000001,
  TransferDst = 0x00000002,
  Uniform = 0x00000010,
  Index = 0x00000040,
  Vertex = 0x00000080,
};

enum class ColorComponent {
  Red = 0x00000001,
  Green = 0x00000002,
  Blue = 0x00000004,
  Alpha = 0x00000008,
};

// sync with VmaMemoryUsage
enum class MemoryUsage {
  Unknown = 0,
  GpuOnly = 1,
  CpuOnly = 2,
  CpuToGpu = 3,
  GpuToCpu = 4,
};
} // namespace aur

namespace aur {
// Sync with VkCullModeFlagBits
enum class CullMode : u32 {
  None = 0,
  Front = 1,
  Back = 2,
  FrontAndBack = 3,
};
} // namespace aur
template <>
struct glz::meta<aur::CullMode> {
  using enum aur::CullMode;
  static constexpr auto value = glz::enumerate(None, Front, Back, FrontAndBack);
};

namespace aur {
// Sync with VkPolygonMode
enum class PolygonMode : u32 {
  Fill = 0,
  Line = 1,
  Point = 2,
  // VK_POLYGON_MODE_FILL_RECTANGLE_NV
};
} // namespace aur
template <>
struct glz::meta<aur::PolygonMode> {
  using enum aur::PolygonMode;
  static constexpr auto value = glz::enumerate(Fill, Line, Point);
};

namespace aur {
// Sync with VkFrontFace
enum class FrontFace : u32 {
  CounterClockwise = 0,
  Clockwise = 1,
};
} // namespace aur
template <>
struct glz::meta<aur::FrontFace> {
  using enum aur::FrontFace;
  static constexpr auto value = glz::enumerate(CounterClockwise, Clockwise);
};

namespace aur {
// Sync with VkBlendFactor
enum class BlendFactor : u32 {
  Zero = 0,
  One = 1,
  SrcColor = 2,
  OneMinusSrcColor = 3,
  DstColor = 4,
  OneMinusDstColor = 5,
  SrcAlpha = 6,
  OneMinusSrcAlpha = 7,
  DstAlpha = 8,
  OneMinusDstAlpha = 9,
  ConstantColor = 10,
  OneMinusConstantColor = 11,
  ConstantAlpha = 12,
  OneMinusConstantAlpha = 13,
  SrcAlphaSaturate = 14,
  Src1Color = 15,
  OneMinusSrc1Color = 16,
  Src1Alpha = 17,
  OneMinusSrc1Alpha = 18,
};
} // namespace aur
template <>
struct glz::meta<aur::BlendFactor> {
  using enum aur::BlendFactor;
  static constexpr auto value = glz::enumerate(
      Zero, One, SrcColor, OneMinusSrcColor, DstColor, OneMinusDstColor, SrcAlpha, OneMinusSrcAlpha, DstAlpha,
      OneMinusDstAlpha, ConstantColor, OneMinusConstantColor, ConstantAlpha, OneMinusConstantAlpha,
      SrcAlphaSaturate, Src1Color, OneMinusSrc1Color, Src1Alpha, OneMinusSrc1Alpha);
};

namespace aur {
// Sync with VkBlendOp (core values only)
enum class BlendOp : u32 {
  // src + dst
  Add = 0,
  // src - dst
  Subtract = 1,
  // dst - src
  ReverseSubtract = 2,
  // min(src, dst)
  Min = 3,
  // max(src, dst)
  Max = 4,
};
} // namespace aur
template <>
struct glz::meta<aur::BlendOp> {
  using enum aur::BlendOp;
  static constexpr auto value = glz::enumerate(Add, Subtract, ReverseSubtract, Min, Max);
};

namespace aur {
// Sync with VkLogicOp
enum class LogicOp : u32 {
  Clear = 0,
  And = 1,
  AndReverse = 2,
  Copy = 3,
  AndInverted = 4,
  NoOp = 5,
  Xor = 6,
  Or = 7,
  Nor = 8,
  Equivalent = 9,
  Invert = 10,
  OrReverse = 11,
  CopyInverted = 12,
  OrInverted = 13,
  Nand = 14,
  Set = 15,
};
} // namespace aur
template <>
struct glz::meta<aur::LogicOp> {
  using enum aur::LogicOp;
  static constexpr auto value =
      glz::enumerate(Clear, And, AndReverse, Copy, AndInverted, NoOp, Xor, Or, Nor, Equivalent, Invert,
                     OrReverse, CopyInverted, OrInverted, Nand, Set);
};

namespace aur {
template <typename TEnum>
u32 toVkFlags(const std::vector<TEnum>& enums);

// template <typename... TEnums>
// u32 toFlagsFold(TEnums... enums) {
//   u32 flags{};
//   ((flags |= static_cast<u32>(enums)), ...);
//   return flags;
// }

} // namespace aur
