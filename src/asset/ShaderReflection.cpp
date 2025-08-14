#include "ShaderReflection.h"

#include "../Logger.h"

namespace aur::asset {
std::optional<ShaderVariableTypeInfo>
ShaderVariableTypeInfo::create(BaseType baseType, uint8_t componentBytes, Signedness signedness,
                               uint8_t vectorSize, uint8_t columnCount) {

  if (vectorSize < 1 || vectorSize > 4 || columnCount < 1 || columnCount > 4)
    return std::nullopt;

  switch (baseType) {
  case BaseType::Unknown:
    log().fatal("Unknown base type in shader parameter type info.");

  case BaseType::Bool:
    // In shader interface blocks, bools are almost always 32-bit for alignment.
    if (componentBytes != 4 || columnCount > 1)
      return std::nullopt;
    if (signedness != Signedness::NotApplicable)
      return std::nullopt;
    break;

  case BaseType::Int:
    if (signedness == Signedness::NotApplicable)
      return std::nullopt;
    if (componentBytes != 1 && componentBytes != 2 && componentBytes != 4 && componentBytes != 8)
      return std::nullopt;
    break;
    if (columnCount > 1)
      return std::nullopt; // No integer matrices in this spec

  case BaseType::Float:
    if (signedness != Signedness::Signed) // Floats are always signed
      return std::nullopt;
    if (componentBytes != 2 && componentBytes != 4 && componentBytes != 8)
      return std::nullopt;
    break;

  case BaseType::Struct:
    if (vectorSize > 1 || columnCount > 1)
      return std::nullopt;
  }

  return ShaderVariableTypeInfo(baseType, componentBytes, signedness, vectorSize, columnCount);
}

// clang-format off
// Constexpr converter from the mnemonic to the factored representation.
constexpr std::optional<ShaderVariableTypeInfo> getFactoredTypeInfo(ShaderVariableTypeMnemonic mnemonic) {
#define FACTOR(base, bytes, sign, vec, col) ShaderVariableTypeInfo::create(ShaderVariableTypeInfo::BaseType::base, bytes, ShaderVariableTypeInfo::Signedness::sign, vec, col)
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

bool ShaderVariableTypeInfo::operator==(const ShaderVariableTypeInfo& other) const {
  return baseType_ == other.baseType_ && componentBytes_ == other.componentBytes_ &&
         signedness_ == other.signedness_ && vectorSize_ == other.vectorSize_ &&
         columnCount_ == other.columnCount_;
}

bool ShaderVariableTypeInfo::operator<(const ShaderVariableTypeInfo& other) const {
  return std::tie(baseType_, componentBytes_, signedness_, vectorSize_, columnCount_) <
         std::tie(other.baseType_, other.componentBytes_, other.signedness_, other.vectorSize_,
                  other.columnCount_);
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
