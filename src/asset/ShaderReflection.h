#pragma once

#include <spirv_cross/spirv_reflect.hpp>

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

struct ShaderVariableTypeInfo {
  enum class BaseType : u8 { Unknown, Struct, Bool, Int, Float };
  enum class Signedness : u8 { NotApplicable, Unsigned, Signed };

  [[nodiscard]] bool isValid() const;
  static ShaderVariableTypeInfo fromSpirV(const spirv_cross::SPIRType& type);

  BaseType baseType{};
  u8 componentBytes{};
  Signedness signedness{};
  u32 vectorSize{};
  u32 columnCnt{};

  [[nodiscard]] std::string toString() const;

  auto operator<=>(const ShaderVariableTypeInfo&) const = default;
};
} // namespace aur::asset
template <>
struct glz::meta<aur::asset::ShaderVariableTypeInfo::BaseType> {
  using enum aur::asset::ShaderVariableTypeInfo::BaseType;
  static constexpr auto value = glz::enumerate(Unknown, Struct, Bool, Int, Float);
};
template <>
struct glz::meta<aur::asset::ShaderVariableTypeInfo::Signedness> {
  using enum aur::asset::ShaderVariableTypeInfo::Signedness;
  static constexpr auto value = glz::enumerate(NotApplicable, Unsigned, Signed);
};

namespace aur::asset {
constexpr ShaderVariableTypeInfo getFactoredTypeInfo(ShaderVariableTypeMnemonic mnemonic);

struct CommonMemberProps {
  spirv_cross::TypeID memberTypeId;
  spirv_cross::SPIRType memberType{spv::Op::OpNop};
  spirv_cross::SPIRType memberBaseType{spv::Op::OpNop};
  //
  ShaderVariableTypeInfo typeInfo{};
  std::string name;
  bool isArray{};
  u32 arraySize{};
};

// A unified structure for all resource block variables
struct ShaderBlockMember {
  ShaderVariableTypeInfo typeInfo{};
  std::string name;

  u32 offset{};
  u64 sizeBytes{};

  bool isArray{};
  u32 arraySize{};

  // Struct properties
  std::vector<ShaderBlockMember> members;

  // ShaderVariable definition is recursive, which implicitly deletes the default <=> operator, because
  // by the time the compiler attempts to synthesize <=> ShaderDefinition is not fully defined, therefore,
  // we have to define it explicitly and compare members of the vector one-by-one.
  std::strong_ordering operator<=>(const ShaderBlockMember& other) const {
    // clang-format off
#define COMPARE_MEMBER(member) if (const auto cmp = member <=> other.member; cmp != std::strong_ordering::equal) return cmp;
    COMPARE_MEMBER(typeInfo);
    COMPARE_MEMBER(name);
    COMPARE_MEMBER(offset);
    COMPARE_MEMBER(sizeBytes);
    COMPARE_MEMBER(isArray);
    COMPARE_MEMBER(arraySize);
#undef COMPARE_MEMBER
    if (const auto cmp = members.size() <=> other.members.size(); cmp != 0) return cmp;
    for (size_t i = 0; i < members.size(); ++i)
      if (const auto cmp = members[i] <=> other.members[i]; cmp != 0) return cmp;
    // clang-format on

    return std::strong_ordering::equal;
  }

  bool operator==(const ShaderBlockMember& other) const { return (*this <=> other) == 0; }
};

struct ShaderInterfaceVariable {
  ShaderVariableTypeInfo typeInfo{};
  std::string name;

  // For shader stage I/O variables: layout(location = N)
  u32 location{static_cast<u32>(-1)};
  bool isFlat{};

  bool isArray{};
  u32 arraySize{};

  // Struct properties (the recursive part)
  std::vector<ShaderInterfaceVariable> members;

  // ShaderInterfaceVariable() = default;
  // ShaderInterfaceVariable(const CommonMemberProps& props)
  //     : typeInfo{props.typeInfo}
  //     , name{props.name}
  //     , isArray{props.isArray}
  //     , arraySize{props.arraySize} {}

  std::strong_ordering operator<=>(const ShaderInterfaceVariable& other) const {
    // clang-format off
#define COMPARE_MEMBER(member) if (const auto cmp = member <=> other.member; cmp != std::strong_ordering::equal) return cmp;
    COMPARE_MEMBER(typeInfo);
    COMPARE_MEMBER(name);
    COMPARE_MEMBER(location);
    COMPARE_MEMBER(isFlat);
    COMPARE_MEMBER(isArray);
    COMPARE_MEMBER(arraySize);
#undef COMPARE_MEMBER
    if (const auto cmp = members.size() <=> other.members.size(); cmp != 0) return cmp;
    for (size_t i = 0; i < members.size(); ++i)
      if (const auto cmp = members[i] <=> other.members[i]; cmp != 0) return cmp;
    // clang-format on

    return std::strong_ordering::equal;
  }

  bool operator==(const ShaderInterfaceVariable& other) const { return (*this <=> other) == 0; }
};

struct ShaderResource {
  u32 set{static_cast<u32>(-1)};
  u32 binding{static_cast<u32>(-1)};
  std::string name;
  u64 sizeBytes{};

  //
  std::vector<ShaderBlockMember> members;

  auto operator<=>(const ShaderResource&) const = default;
};

using SetNo = u32;
using BindingNo = u32;
using LocationNo = u32;
struct ShaderStageSchema {
  std::map<LocationNo, ShaderInterfaceVariable> inputs;
  std::map<SetNo, std::map<BindingNo, ShaderResource>> uniformsBuffers;
  // std::map<DescriptorKey, StorageBufferSchema> storageBuffers;
  // std::map<DescriptorKey, SampledImage> samplesImages;
  // std::map<DescriptorKey, StorageImage> storageImages;
  // std::map<DescriptorKey, Sampler> samplers;
  // std::map<DescriptorKey, AccelerationStructure> accelerationStructures;
  std::map<LocationNo, ShaderInterfaceVariable> outputs;
};

using SpirV = std::vector<u32>;
[[nodiscard]] ShaderStageSchema reflectShaderStageSchema(const SpirV& spirV);

} // namespace aur::asset
