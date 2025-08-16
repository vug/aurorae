#include "ShaderReflection.h"

#include <spirv_cross/spirv_reflect.hpp>

#include "../Logger.h"

namespace aur::asset {
bool ShaderVariableTypeInfo::isValid() const {

  if (vectorSize < 1 || vectorSize > 4 || columnCnt < 1 || columnCnt > 4)
    return false;

  switch (baseType) {
  case BaseType::Unknown:
    log().fatal("Unknown base type in shader parameter type info.");

  case BaseType::Bool:
    // In shader interface blocks, bools are almost always 32-bit for alignment.
    if (componentBytes != 4 || columnCnt > 1)
      return false;
    if (signedness != Signedness::NotApplicable)
      return false;

  case BaseType::Int:
    if (signedness == Signedness::NotApplicable)
      return false;
    if (componentBytes != 1 && componentBytes != 2 && componentBytes != 4 && componentBytes != 8)
      return false;
    if (columnCnt > 1)
      return false; // No integer matrices in this spec yet

  case BaseType::Float:
    if (signedness != Signedness::Signed) // Floats are always signed
      return false;
    if (componentBytes != 2 && componentBytes != 4 && componentBytes != 8)
      return false;
    break;

  case BaseType::Struct:
    if (vectorSize > 1 || columnCnt > 1)
      return false;
  }

  return true;
}

namespace {
ShaderVariableTypeInfo::BaseType spirvTypeToShaderVariableBaseType(const spirv_cross::SPIRType& type) {
  switch (type.basetype) {
  case spirv_cross::SPIRType::Boolean:
    return ShaderVariableTypeInfo::BaseType::Bool;
  case spirv_cross::SPIRType::SByte:
  case spirv_cross::SPIRType::Short:
  case spirv_cross::SPIRType::Int:
  case spirv_cross::SPIRType::Int64:
  case spirv_cross::SPIRType::UByte:
  case spirv_cross::SPIRType::UShort:
  case spirv_cross::SPIRType::UInt:
  case spirv_cross::SPIRType::UInt64:
    return ShaderVariableTypeInfo::BaseType::Int;
  case spirv_cross::SPIRType::Half:
  case spirv_cross::SPIRType::Float:
  case spirv_cross::SPIRType::Double:
    return ShaderVariableTypeInfo::BaseType::Float;
  case spirv_cross::SPIRType::Unknown:
    return ShaderVariableTypeInfo::BaseType::Unknown;
  case spirv_cross::SPIRType::Struct:
    return ShaderVariableTypeInfo::BaseType::Struct;
  default:
    log().fatal("Unknown SPIRType for ShaderVariableTypeInfo::BaseType.");
  }
  std::unreachable();
}

u8 spirvTypeToShaderVariableComponentBytes(const spirv_cross::SPIRType& type) {
  switch (type.basetype) {
  case spirv_cross::SPIRType::SByte:
  case spirv_cross::SPIRType::UByte:
    return 1;
  case spirv_cross::SPIRType::Short:
  case spirv_cross::SPIRType::UShort:
  case spirv_cross::SPIRType::Half:
    return 2;
  case spirv_cross::SPIRType::Boolean:
  case spirv_cross::SPIRType::Int:
  case spirv_cross::SPIRType::UInt:
  case spirv_cross::SPIRType::Float:
    return 4;
  case spirv_cross::SPIRType::Int64:
  case spirv_cross::SPIRType::UInt64:
  case spirv_cross::SPIRType::Double:
    return 8;
  case spirv_cross::SPIRType::Unknown:
  case spirv_cross::SPIRType::Struct:
    return static_cast<u8>(-1);
  default:
    log().fatal("Unknown SPIRType for ShaderVariableTypeInfo::ComponentBytes.");
  }
  std::unreachable();
}

ShaderVariableTypeInfo::Signedness spirvTypeToShaderVariableSignedness(const spirv_cross::SPIRType& type) {

  switch (type.basetype) {
  case spirv_cross::SPIRType::Boolean:
    return ShaderVariableTypeInfo::Signedness::NotApplicable;
  case spirv_cross::SPIRType::SByte:
  case spirv_cross::SPIRType::Short:
  case spirv_cross::SPIRType::Int:
  case spirv_cross::SPIRType::Int64:
  case spirv_cross::SPIRType::Half:
  case spirv_cross::SPIRType::Float:
  case spirv_cross::SPIRType::Double:
    return ShaderVariableTypeInfo::Signedness::Signed;
  case spirv_cross::SPIRType::UByte:
  case spirv_cross::SPIRType::UShort:
  case spirv_cross::SPIRType::UInt:
  case spirv_cross::SPIRType::UInt64:
    return ShaderVariableTypeInfo::Signedness::Unsigned;
  case spirv_cross::SPIRType::Unknown:
  case spirv_cross::SPIRType::Struct:
    return ShaderVariableTypeInfo::Signedness::NotApplicable;
  default:
    log().fatal("Unknown SPIRType for ShaderVariableTypeInfo::BaseType.");
  }
  std::unreachable();
}

const char* spirVBaseTypeToString(const spirv_cross::SPIRType& type) {
  switch (type.basetype) {
  case spirv_cross::SPIRType::Unknown:
    return "Unknown";
  case spirv_cross::SPIRType::Void:
    return "Void";
  case spirv_cross::SPIRType::Boolean:
    return "Boolean";
  case spirv_cross::SPIRType::SByte:
    return "SByte";
  case spirv_cross::SPIRType::UByte:
    return "UByte";
  case spirv_cross::SPIRType::Short:
    return "Short";
  case spirv_cross::SPIRType::UShort:
    return "UShort";
  case spirv_cross::SPIRType::Int:
    return "Int";
  case spirv_cross::SPIRType::UInt:
    return "UInt";
  case spirv_cross::SPIRType::Int64:
    return "Int64";
  case spirv_cross::SPIRType::UInt64:
    return "UInt64";
  case spirv_cross::SPIRType::Half:
    return "Half";
  case spirv_cross::SPIRType::Float:
    return "Float";
  case spirv_cross::SPIRType::Double:
    return "Double";
  case spirv_cross::SPIRType::Struct:
    return "Struct";
  case spirv_cross::SPIRType::Image:
    return "Image";
  case spirv_cross::SPIRType::SampledImage:
    return "SampledImage";
  case spirv_cross::SPIRType::Sampler:
    return "Sampler";
  case spirv_cross::SPIRType::AccelerationStructure:
    return "AccelerationStructure";
    // Add other types as needed
  default:
    return "Unsupported";
  }
}

} // namespace

ShaderVariableTypeInfo ShaderVariableTypeInfo::fromSpirV(const spirv_cross::SPIRType& type) {
  return {.baseType = spirvTypeToShaderVariableBaseType(type),
          .componentBytes = spirvTypeToShaderVariableComponentBytes(type),
          .signedness = spirvTypeToShaderVariableSignedness(type),
          .vectorSize = type.vecsize,
          .columnCnt = type.columns};
}

// clang-format off
// Constexpr converter from the mnemonic to the factored representation.
constexpr ShaderVariableTypeInfo getFactoredTypeInfo(ShaderVariableTypeMnemonic mnemonic) {
#define FACTOR(base, bytes, sign, vec, col) ShaderVariableTypeInfo{ShaderVariableTypeInfo::BaseType::base, bytes, ShaderVariableTypeInfo::Signedness::sign, vec, col}
  switch (mnemonic) {
  case ShaderVariableTypeMnemonic::Unknown:      return FACTOR(Unknown, 0, NotApplicable, 0, 0);
  case ShaderVariableTypeMnemonic::Struct:       return FACTOR(Struct,  0, NotApplicable, 1, 1);

  // Bools are logically 1-bit, but physically 32-bit in interface blocks.
  case ShaderVariableTypeMnemonic::bool1:        return FACTOR(Bool,    4, NotApplicable, 1, 1);
  case ShaderVariableTypeMnemonic::bool2:        return FACTOR(Bool,    4, NotApplicable, 2, 1);
  case ShaderVariableTypeMnemonic::bool3:        return FACTOR(Bool,    4, NotApplicable, 3, 1);
  case ShaderVariableTypeMnemonic::bool4:        return FACTOR(Bool,    4, NotApplicable, 4, 1);

  // 8-bit Integers
  case ShaderVariableTypeMnemonic::int8_t1:      return FACTOR(Int,     1, Signed,        1, 1);
  case ShaderVariableTypeMnemonic::int8_t2:      return FACTOR(Int,     1, Signed,        2, 1);
  case ShaderVariableTypeMnemonic::int8_t3:      return FACTOR(Int,     1, Signed,        3, 1);
  case ShaderVariableTypeMnemonic::int8_t4:      return FACTOR(Int,     1, Signed,        4, 1);
  case ShaderVariableTypeMnemonic::uint8_t1:     return FACTOR(Int,     1, Unsigned,      1, 1);
  case ShaderVariableTypeMnemonic::uint8_t2:     return FACTOR(Int,     1, Unsigned,      2, 1);
  case ShaderVariableTypeMnemonic::uint8_t3:     return FACTOR(Int,     1, Unsigned,      3, 1);
  case ShaderVariableTypeMnemonic::uint8_t4:     return FACTOR(Int,     1, Unsigned,      4, 1);

  // 16-bit Integers
  case ShaderVariableTypeMnemonic::int16_t1:     return FACTOR(Int,     2, Signed,        1, 1);
  case ShaderVariableTypeMnemonic::int16_t2:     return FACTOR(Int,     2, Signed,        2, 1);
  case ShaderVariableTypeMnemonic::int16_t3:     return FACTOR(Int,     2, Signed,        3, 1);
  case ShaderVariableTypeMnemonic::int16_t4:     return FACTOR(Int,     2, Signed,        4, 1);
  case ShaderVariableTypeMnemonic::uint16_t1:    return FACTOR(Int,     2, Unsigned,      1, 1);
  case ShaderVariableTypeMnemonic::uint16_t2:    return FACTOR(Int,     2, Unsigned,      2, 1);
  case ShaderVariableTypeMnemonic::uint16_t3:    return FACTOR(Int,     2, Unsigned,      3, 1);
  case ShaderVariableTypeMnemonic::uint16_t4:    return FACTOR(Int,     2, Unsigned,      4, 1);

  // 32-bit Integers
  case ShaderVariableTypeMnemonic::int32_t1:     return FACTOR(Int,     4, Signed,        1, 1);
  case ShaderVariableTypeMnemonic::int32_t2:     return FACTOR(Int,     4, Signed,        2, 1);
  case ShaderVariableTypeMnemonic::int32_t3:     return FACTOR(Int,     4, Signed,        3, 1);
  case ShaderVariableTypeMnemonic::int32_t4:     return FACTOR(Int,     4, Signed,        4, 1);
  case ShaderVariableTypeMnemonic::uint32_t1:    return FACTOR(Int,     4, Unsigned,      1, 1);
  case ShaderVariableTypeMnemonic::uint32_t2:    return FACTOR(Int,     4, Unsigned,      2, 1);
  case ShaderVariableTypeMnemonic::uint32_t3:    return FACTOR(Int,     4, Unsigned,      3, 1);
  case ShaderVariableTypeMnemonic::uint32_t4:    return FACTOR(Int,     4, Unsigned,      4, 1);

  // 64-bit Integers
  case ShaderVariableTypeMnemonic::int64_t1:     return FACTOR(Int,     8, Signed,        1, 1);
  case ShaderVariableTypeMnemonic::int64_t2:     return FACTOR(Int,     8, Signed,        2, 1);
  case ShaderVariableTypeMnemonic::int64_t3:     return FACTOR(Int,     8, Signed,        3, 1);
  case ShaderVariableTypeMnemonic::int64_t4:     return FACTOR(Int,     8, Signed,        4, 1);
  case ShaderVariableTypeMnemonic::uint64_t1:    return FACTOR(Int,     8, Unsigned,      1, 1);
  case ShaderVariableTypeMnemonic::uint64_t2:    return FACTOR(Int,     8, Unsigned,      2, 1);
  case ShaderVariableTypeMnemonic::uint64_t3:    return FACTOR(Int,     8, Unsigned,      3, 1);
  case ShaderVariableTypeMnemonic::uint64_t4:    return FACTOR(Int,     8, Unsigned,      4, 1);

  // 16-bit Floats (Half)
  case ShaderVariableTypeMnemonic::float16_t1:   return FACTOR(Float,   2, Signed,        1, 1);
  case ShaderVariableTypeMnemonic::float16_t2:   return FACTOR(Float,   2, Signed,        2, 1);
  case ShaderVariableTypeMnemonic::float16_t3:   return FACTOR(Float,   2, Signed,        3, 1);
  case ShaderVariableTypeMnemonic::float16_t4:   return FACTOR(Float,   2, Signed,        4, 1);

  // 32-bit Floats (Float)
  case ShaderVariableTypeMnemonic::float32_t1:   return FACTOR(Float,   4, Signed,        1, 1);
  case ShaderVariableTypeMnemonic::float32_t2:   return FACTOR(Float,   4, Signed,        2, 1);
  case ShaderVariableTypeMnemonic::float32_t3:   return FACTOR(Float,   4, Signed,        3, 1);
  case ShaderVariableTypeMnemonic::float32_t4:   return FACTOR(Float,   4, Signed,        4, 1);

  // 64-bit Floats (Double)
  case ShaderVariableTypeMnemonic::float64_t1:   return FACTOR(Float,   8, Signed,        1, 1);
  case ShaderVariableTypeMnemonic::float64_t2:   return FACTOR(Float,   8, Signed,        2, 1);
  case ShaderVariableTypeMnemonic::float64_t3:   return FACTOR(Float,   8, Signed,        3, 1);
  case ShaderVariableTypeMnemonic::float64_t4:   return FACTOR(Float,   8, Signed,        4, 1);

  // 16-bit Matrices
  case ShaderVariableTypeMnemonic::float16_t2x2: return FACTOR(Float,   2, Signed,        2, 2);
  case ShaderVariableTypeMnemonic::float16_t2x3: return FACTOR(Float,   2, Signed,        3, 2);
  case ShaderVariableTypeMnemonic::float16_t2x4: return FACTOR(Float,   2, Signed,        4, 2);
  case ShaderVariableTypeMnemonic::float16_t3x2: return FACTOR(Float,   2, Signed,        2, 3);
  case ShaderVariableTypeMnemonic::float16_t3x3: return FACTOR(Float,   2, Signed,        3, 3);
  case ShaderVariableTypeMnemonic::float16_t3x4: return FACTOR(Float,   2, Signed,        4, 3);
  case ShaderVariableTypeMnemonic::float16_t4x2: return FACTOR(Float,   2, Signed,        2, 4);
  case ShaderVariableTypeMnemonic::float16_t4x3: return FACTOR(Float,   2, Signed,        3, 4);
  case ShaderVariableTypeMnemonic::float16_t4x4: return FACTOR(Float,   2, Signed,        4, 4);

  // 32-bit Matrices
  case ShaderVariableTypeMnemonic::float32_t2x2: return FACTOR(Float,   4, Signed,        2, 2);
  case ShaderVariableTypeMnemonic::float32_t2x3: return FACTOR(Float,   4, Signed,        3, 2);
  case ShaderVariableTypeMnemonic::float32_t2x4: return FACTOR(Float,   4, Signed,        4, 2);
  case ShaderVariableTypeMnemonic::float32_t3x2: return FACTOR(Float,   4, Signed,        2, 3);
  case ShaderVariableTypeMnemonic::float32_t3x3: return FACTOR(Float,   4, Signed,        3, 3);
  case ShaderVariableTypeMnemonic::float32_t3x4: return FACTOR(Float,   4, Signed,        4, 3);
  case ShaderVariableTypeMnemonic::float32_t4x2: return FACTOR(Float,   4, Signed,        2, 4);
  case ShaderVariableTypeMnemonic::float32_t4x3: return FACTOR(Float,   4, Signed,        3, 4);
  case ShaderVariableTypeMnemonic::float32_t4x4: return FACTOR(Float,   4, Signed,        4, 4);

  // 64-bit Matrices
  case ShaderVariableTypeMnemonic::float64_t2x2: return FACTOR(Float,   4, Signed,        2, 2);
  case ShaderVariableTypeMnemonic::float64_t2x3: return FACTOR(Float,   4, Signed,        3, 2);
  case ShaderVariableTypeMnemonic::float64_t2x4: return FACTOR(Float,   4, Signed,        4, 2);
  case ShaderVariableTypeMnemonic::float64_t3x2: return FACTOR(Float,   4, Signed,        2, 3);
  case ShaderVariableTypeMnemonic::float64_t3x3: return FACTOR(Float,   4, Signed,        3, 3);
  case ShaderVariableTypeMnemonic::float64_t3x4: return FACTOR(Float,   4, Signed,        4, 3);
  case ShaderVariableTypeMnemonic::float64_t4x2: return FACTOR(Float,   4, Signed,        2, 4);
  case ShaderVariableTypeMnemonic::float64_t4x3: return FACTOR(Float,   4, Signed,        3, 4);
  case ShaderVariableTypeMnemonic::float64_t4x4: return FACTOR(Float,   4, Signed,        4, 4);
  }
  std::unreachable();
#undef FACTOR
}
// clang-format on

std::string ShaderVariableTypeInfo::toString() const {
  const std::string signPrefix = (signedness == ShaderVariableTypeInfo::Signedness::Unsigned) ? "U" : "";
  const std::string typeName = glz::write<glz::opts{.raw = true}>(baseType).value_or("unknown");
  const u8 bitCount = componentBytes * 8;
  const std::string matrixSuffix = columnCnt > 1 ? std::format("x{}", columnCnt) : "";
  return std::format("{}{}{}_t{}{}", signPrefix, typeName, bitCount, vectorSize, matrixSuffix);
}

bool ShaderVariable::operator==(const ShaderVariable& other) const {

  if (category != other.category) {
    return false;
  }

  switch (category) {
  case ShaderVariableCategory::StageInput:
  case ShaderVariableCategory::StageOutput:
    // For stage I/O validation, we compare location, type, and array properties.
    // Name is ignored as it can differ between stages or be compiled away.
    return location == other.location && typeInfo == other.typeInfo && isArray == other.isArray &&
           arraySize == other.arraySize;

  case ShaderVariableCategory::UniformBufferMember:
    // For uniform members, all properties should ideally match.
    return name == other.name && typeInfo == other.typeInfo && set == other.set && binding == other.binding &&
           offset == other.offset && sizeBytes == other.sizeBytes && isArray == other.isArray &&
           arraySize == other.arraySize;

  default:
    log().fatal("comparison not implemented for category: {}", static_cast<u32>(category));
  }
}

bool ShaderVariable::operator<(const ShaderVariable& other) const {
  return std::tie(category, set, binding, location, offset, name) <
         std::tie(other.category, other.set, other.binding, other.location, other.offset, other.name);
}

} // namespace aur::asset
