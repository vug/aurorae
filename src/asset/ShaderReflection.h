3#pragma once

#include <optional>

#include "../Utils.h"

namespace aur::asset {
// clang-format off
enum class ShaderVariableTypeMnemonic : u8 {
  Unknown,
  Struct,

  // Boolean Scalar & Vector Types
  // Don't use these, sizeof(bool) in GLSL and HLSL are 4-bytes
  bool1, bool2,  bool3, bool4,

  // Integer Scalar & Vector Types
  int8_t1,   int8_t2,   int8_t3,   int8_t4,
  uint8_t1,  uint8_t2,  uint8_t3,  uint8_t4,
  int16_t1,  int16_t2,  int16_t3,  int16_t4,
  uint16_t1, uint16_t2, uint16_t3, uint16_t4,
  int32_t1,  int32_t2,  int32_t3,  int32_t4,
  uint32_t1,  uint32_t2,  uint32_t3,  uint32_t4,
  int64_t1,  int64_t2,  int64_t3,  int64_t4,
  uint64_t1,  uint64_t2,  uint64_t3,  uint64_t4,

  // Floating-Point Scalar & Vector types
  float16_t1, float16_t2, float16_t3, float16_t4,
  float32_t1, float32_t2, float32_t3, float32_t4,
  float64_t1, float64_t2, float64_t3, float64_t4,

  // Floating-Point Matrix Types
  float16_t2x2, float16_t2x3, float16_t2x4,
  float16_t3x2, float16_t3x3, float16_t3x4,
  float16_t4x2, float16_t4x3, float16_t4x4,

  float32_t2x2, float32_t2x3, float32_t2x4,
  float32_t3x2, float32_t3x3, float32_t3x4,
  float32_t4x2, float32_t4x3, float32_t4x4,

  float64_t2x2, float64_t2x3, float64_t2x4,
  float64_t3x2, float64_t3x3, float64_t3x4,
  float64_t4x2, float64_t4x3, float64_t4x4,
};
// clang-format on

// An immutable, always-valid class representing the decomposed properties of a shader type.
class ShaderVariableTypeInfo {
public:
  enum class BaseType : u8 { Unknown, Struct, Bool, Int, Float };
  enum class Signedness : u8 { NotApplicable, Unsigned, Signed };

  // creates an invalid object
  ShaderVariableTypeInfo() = default;

  // Public factory method to ensure only valid instances are created.
  static std::optional<ShaderVariableTypeInfo> create(BaseType baseType, u8 componentBytes,
                                                      Signedness signedness, u8 vectorSize, u8 columnCount);

  [[nodiscard]] constexpr BaseType getBaseType() const { return baseType_; }
  [[nodiscard]] constexpr u8 getComponentBytes() const { return componentBytes_; }
  [[nodiscard]] constexpr Signedness getSignedness() const { return signedness_; }
  [[nodiscard]] constexpr u8 getVectorSize() const { return vectorSize_; }
  [[nodiscard]] constexpr u8 getColumnCount() const { return columnCount_; }
  [[nodiscard]] constexpr bool isMatrix() const { return columnCount_ > 1; }

  bool operator<(const ShaderVariableTypeInfo& other) const;
  bool operator==(const ShaderVariableTypeInfo& other) const;

private:
  constexpr ShaderVariableTypeInfo(BaseType baseType, u8 componentBytes, Signedness signedness, u8 vectorSize,
                                   u8 columnCount)
      : baseType_(baseType)
      , componentBytes_(componentBytes)
      , signedness_(signedness)
      , vectorSize_(vectorSize)
      , columnCount_(columnCount) {}

  BaseType baseType_{};
  uint8_t componentBytes_{};
  Signedness signedness_{};
  uint8_t vectorSize_{};
  uint8_t columnCount_{};
};

constexpr std::optional<ShaderVariableTypeInfo> getFactoredTypeInfo(ShaderVariableTypeMnemonic mnemonic);

enum class ShaderVariableCategory {
  Unknown,
  UniformBufferMember,
  StageInput,
  StageOutput,
  // PushConstant,
  // Texture
  // Sampler,
  // StorageBuffer,
  // ...
};

// A unified structure for all shader variables
struct ShaderVariable {
  std::string name;
  ShaderVariableCategory category{};
  ShaderVariableTypeInfo typeInfo{};

  // For stage I/O variables: layout(location = N)
  u32 location{};

  // For resources like uniform buffers, samplers: layout(set = N, binding = N)
  u32 set{};
  u32 binding{};

  // For members of blocks like uniform buffers
  u32 offset{};
  u64 sizeBytes{};

  // Array properties
  bool isArray{};
  u32 arraySize{};

  // For validation and equality checks. The logic depends on the category.
  bool operator==(const ShaderVariable& other) const;
  // For sorting to establish a canonical order.
  // We sort by category, then resource identifiers (set, binding), then I/O location,
  // then offset within a block, and finally by name for stability.
  bool operator<(const ShaderVariable& other) const;
};

} // namespace aur::asset