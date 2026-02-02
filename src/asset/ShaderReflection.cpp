#include "ShaderReflection.h"

#include "../Logger.h"
#include "Material.h"
#include "ShaderStage.h"

namespace aur::asset {
bool ShaderVariableTypeInfo::isValid() const {

  if (vectorSize < 1 || vectorSize > 4 || columnCnt < 1 || columnCnt > 4)
    return false;

  switch (baseType) {
  case BaseType::Unknown:
    log().fatal("Unknown base type in shader parameter type info.");

  case BaseType::Bool:
    // In shader interface blocks, booleans are almost always 32-bit for alignment.
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

} // namespace

ShaderVariableTypeInfo ShaderVariableTypeInfo::fromSpirV(const spirv_cross::SPIRType& type) {
  return {.baseType = spirvTypeToShaderVariableBaseType(type),
          .componentBytes = static_cast<u8>(type.width / 8),
          .signedness = spirvTypeToShaderVariableSignedness(type),
          .vectorSize = type.vecsize,
          .columnCnt = type.columns};
}

// clang-format off
// Constexpr converter from the mnemonic to the factored representation.
constexpr ShaderVariableTypeInfo getFactoredTypeInfo(ShaderVariableType mnemonic) {
#define FACTOR(base, bytes, sign, vec, col) ShaderVariableTypeInfo{ShaderVariableTypeInfo::BaseType::base, bytes, ShaderVariableTypeInfo::Signedness::sign, vec, col}
  switch (mnemonic) {
  case ShaderVariableType::Unknown:      return FACTOR(Unknown, 0, NotApplicable, 0, 0);
  case ShaderVariableType::Struct:       return FACTOR(Struct,  0, NotApplicable, 1, 1);

  // Bools are logically 1-bit, but physically 32-bit in interface blocks.
  case ShaderVariableType::bool1:        return FACTOR(Bool,    4, NotApplicable, 1, 1);
  case ShaderVariableType::bool2:        return FACTOR(Bool,    4, NotApplicable, 2, 1);
  case ShaderVariableType::bool3:        return FACTOR(Bool,    4, NotApplicable, 3, 1);
  case ShaderVariableType::bool4:        return FACTOR(Bool,    4, NotApplicable, 4, 1);

  // 8-bit Integers
  case ShaderVariableType::int8_t1:      return FACTOR(Int,     1, Signed,        1, 1);
  case ShaderVariableType::int8_t2:      return FACTOR(Int,     1, Signed,        2, 1);
  case ShaderVariableType::int8_t3:      return FACTOR(Int,     1, Signed,        3, 1);
  case ShaderVariableType::int8_t4:      return FACTOR(Int,     1, Signed,        4, 1);
  case ShaderVariableType::uint8_t1:     return FACTOR(Int,     1, Unsigned,      1, 1);
  case ShaderVariableType::uint8_t2:     return FACTOR(Int,     1, Unsigned,      2, 1);
  case ShaderVariableType::uint8_t3:     return FACTOR(Int,     1, Unsigned,      3, 1);
  case ShaderVariableType::uint8_t4:     return FACTOR(Int,     1, Unsigned,      4, 1);

  // 16-bit Integers
  case ShaderVariableType::int16_t1:     return FACTOR(Int,     2, Signed,        1, 1);
  case ShaderVariableType::int16_t2:     return FACTOR(Int,     2, Signed,        2, 1);
  case ShaderVariableType::int16_t3:     return FACTOR(Int,     2, Signed,        3, 1);
  case ShaderVariableType::int16_t4:     return FACTOR(Int,     2, Signed,        4, 1);
  case ShaderVariableType::uint16_t1:    return FACTOR(Int,     2, Unsigned,      1, 1);
  case ShaderVariableType::uint16_t2:    return FACTOR(Int,     2, Unsigned,      2, 1);
  case ShaderVariableType::uint16_t3:    return FACTOR(Int,     2, Unsigned,      3, 1);
  case ShaderVariableType::uint16_t4:    return FACTOR(Int,     2, Unsigned,      4, 1);

  // 32-bit Integers
  case ShaderVariableType::int32_t1:     return FACTOR(Int,     4, Signed,        1, 1);
  case ShaderVariableType::int32_t2:     return FACTOR(Int,     4, Signed,        2, 1);
  case ShaderVariableType::int32_t3:     return FACTOR(Int,     4, Signed,        3, 1);
  case ShaderVariableType::int32_t4:     return FACTOR(Int,     4, Signed,        4, 1);
  case ShaderVariableType::uint32_t1:    return FACTOR(Int,     4, Unsigned,      1, 1);
  case ShaderVariableType::uint32_t2:    return FACTOR(Int,     4, Unsigned,      2, 1);
  case ShaderVariableType::uint32_t3:    return FACTOR(Int,     4, Unsigned,      3, 1);
  case ShaderVariableType::uint32_t4:    return FACTOR(Int,     4, Unsigned,      4, 1);

  // 64-bit Integers
  case ShaderVariableType::int64_t1:     return FACTOR(Int,     8, Signed,        1, 1);
  case ShaderVariableType::int64_t2:     return FACTOR(Int,     8, Signed,        2, 1);
  case ShaderVariableType::int64_t3:     return FACTOR(Int,     8, Signed,        3, 1);
  case ShaderVariableType::int64_t4:     return FACTOR(Int,     8, Signed,        4, 1);
  case ShaderVariableType::uint64_t1:    return FACTOR(Int,     8, Unsigned,      1, 1);
  case ShaderVariableType::uint64_t2:    return FACTOR(Int,     8, Unsigned,      2, 1);
  case ShaderVariableType::uint64_t3:    return FACTOR(Int,     8, Unsigned,      3, 1);
  case ShaderVariableType::uint64_t4:    return FACTOR(Int,     8, Unsigned,      4, 1);

  // 16-bit Floats (Half)
  case ShaderVariableType::float16_t1:   return FACTOR(Float,   2, Signed,        1, 1);
  case ShaderVariableType::float16_t2:   return FACTOR(Float,   2, Signed,        2, 1);
  case ShaderVariableType::float16_t3:   return FACTOR(Float,   2, Signed,        3, 1);
  case ShaderVariableType::float16_t4:   return FACTOR(Float,   2, Signed,        4, 1);

  // 32-bit Floats (Float)
  case ShaderVariableType::float32_t1:   return FACTOR(Float,   4, Signed,        1, 1);
  case ShaderVariableType::float32_t2:   return FACTOR(Float,   4, Signed,        2, 1);
  case ShaderVariableType::float32_t3:   return FACTOR(Float,   4, Signed,        3, 1);
  case ShaderVariableType::float32_t4:   return FACTOR(Float,   4, Signed,        4, 1);

  // 64-bit Floats (Double)
  case ShaderVariableType::float64_t1:   return FACTOR(Float,   8, Signed,        1, 1);
  case ShaderVariableType::float64_t2:   return FACTOR(Float,   8, Signed,        2, 1);
  case ShaderVariableType::float64_t3:   return FACTOR(Float,   8, Signed,        3, 1);
  case ShaderVariableType::float64_t4:   return FACTOR(Float,   8, Signed,        4, 1);

  // 16-bit Matrices
  case ShaderVariableType::float16_t2x2: return FACTOR(Float,   2, Signed,        2, 2);
  case ShaderVariableType::float16_t2x3: return FACTOR(Float,   2, Signed,        3, 2);
  case ShaderVariableType::float16_t2x4: return FACTOR(Float,   2, Signed,        4, 2);
  case ShaderVariableType::float16_t3x2: return FACTOR(Float,   2, Signed,        2, 3);
  case ShaderVariableType::float16_t3x3: return FACTOR(Float,   2, Signed,        3, 3);
  case ShaderVariableType::float16_t3x4: return FACTOR(Float,   2, Signed,        4, 3);
  case ShaderVariableType::float16_t4x2: return FACTOR(Float,   2, Signed,        2, 4);
  case ShaderVariableType::float16_t4x3: return FACTOR(Float,   2, Signed,        3, 4);
  case ShaderVariableType::float16_t4x4: return FACTOR(Float,   2, Signed,        4, 4);

  // 32-bit Matrices
  case ShaderVariableType::float32_t2x2: return FACTOR(Float,   4, Signed,        2, 2);
  case ShaderVariableType::float32_t2x3: return FACTOR(Float,   4, Signed,        3, 2);
  case ShaderVariableType::float32_t2x4: return FACTOR(Float,   4, Signed,        4, 2);
  case ShaderVariableType::float32_t3x2: return FACTOR(Float,   4, Signed,        2, 3);
  case ShaderVariableType::float32_t3x3: return FACTOR(Float,   4, Signed,        3, 3);
  case ShaderVariableType::float32_t3x4: return FACTOR(Float,   4, Signed,        4, 3);
  case ShaderVariableType::float32_t4x2: return FACTOR(Float,   4, Signed,        2, 4);
  case ShaderVariableType::float32_t4x3: return FACTOR(Float,   4, Signed,        3, 4);
  case ShaderVariableType::float32_t4x4: return FACTOR(Float,   4, Signed,        4, 4);

  // 64-bit Matrices
  case ShaderVariableType::float64_t2x2: return FACTOR(Float,   4, Signed,        2, 2);
  case ShaderVariableType::float64_t2x3: return FACTOR(Float,   4, Signed,        3, 2);
  case ShaderVariableType::float64_t2x4: return FACTOR(Float,   4, Signed,        4, 2);
  case ShaderVariableType::float64_t3x2: return FACTOR(Float,   4, Signed,        2, 3);
  case ShaderVariableType::float64_t3x3: return FACTOR(Float,   4, Signed,        3, 3);
  case ShaderVariableType::float64_t3x4: return FACTOR(Float,   4, Signed,        4, 3);
  case ShaderVariableType::float64_t4x2: return FACTOR(Float,   4, Signed,        2, 4);
  case ShaderVariableType::float64_t4x3: return FACTOR(Float,   4, Signed,        3, 4);
  case ShaderVariableType::float64_t4x4: return FACTOR(Float,   4, Signed,        4, 4);
  }
  std::unreachable();
#undef FACTOR
}
// clang-format on

// clang-format off
// Constexpr converter from the factored representation to the mnemonic.
constexpr ShaderVariableType getShaderVariableType(const ShaderVariableTypeInfo& typeInfo) {
  using BaseType = ShaderVariableTypeInfo::BaseType;
  using Signedness = ShaderVariableTypeInfo::Signedness;

  // Handle special cases first
  if (typeInfo.baseType == BaseType::Unknown)
    return ShaderVariableType::Unknown;
  if (typeInfo.baseType == BaseType::Struct)
    return ShaderVariableType::Struct;

  // Handle booleans (always 4 bytes in interface blocks)
  if (typeInfo.baseType == BaseType::Bool && typeInfo.componentBytes == 4 && typeInfo.columnCnt == 1) {
    switch (typeInfo.vectorSize) {
    case 1: return ShaderVariableType::bool1;
    case 2: return ShaderVariableType::bool2;
    case 3: return ShaderVariableType::bool3;
    case 4: return ShaderVariableType::bool4;
    }
  }

  // Handle integers (scalars and vectors only)
  if (typeInfo.baseType == BaseType::Int && typeInfo.columnCnt == 1) {
    if (typeInfo.signedness == Signedness::Signed) {
      switch (typeInfo.componentBytes) {
      case 1:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::int8_t1;
        case 2: return ShaderVariableType::int8_t2;
        case 3: return ShaderVariableType::int8_t3;
        case 4: return ShaderVariableType::int8_t4;
        }
        break;
      case 2:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::int16_t1;
        case 2: return ShaderVariableType::int16_t2;
        case 3: return ShaderVariableType::int16_t3;
        case 4: return ShaderVariableType::int16_t4;
        }
        break;
      case 4:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::int32_t1;
        case 2: return ShaderVariableType::int32_t2;
        case 3: return ShaderVariableType::int32_t3;
        case 4: return ShaderVariableType::int32_t4;
        }
        break;
      case 8:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::int64_t1;
        case 2: return ShaderVariableType::int64_t2;
        case 3: return ShaderVariableType::int64_t3;
        case 4: return ShaderVariableType::int64_t4;
        }
        break;
      }
    } else if (typeInfo.signedness == Signedness::Unsigned) {
      switch (typeInfo.componentBytes) {
      case 1:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::uint8_t1;
        case 2: return ShaderVariableType::uint8_t2;
        case 3: return ShaderVariableType::uint8_t3;
        case 4: return ShaderVariableType::uint8_t4;
        }
        break;
      case 2:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::uint16_t1;
        case 2: return ShaderVariableType::uint16_t2;
        case 3: return ShaderVariableType::uint16_t3;
        case 4: return ShaderVariableType::uint16_t4;
        }
        break;
      case 4:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::uint32_t1;
        case 2: return ShaderVariableType::uint32_t2;
        case 3: return ShaderVariableType::uint32_t3;
        case 4: return ShaderVariableType::uint32_t4;
        }
        break;
      case 8:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::uint64_t1;
        case 2: return ShaderVariableType::uint64_t2;
        case 3: return ShaderVariableType::uint64_t3;
        case 4: return ShaderVariableType::uint64_t4;
        }
        break;
      }
    }
  }

  // Handle floats (scalars, vectors, and matrices)
  if (typeInfo.baseType == BaseType::Float && typeInfo.signedness == Signedness::Signed) {
    // Scalar and vector floats (columnCnt == 1)
    if (typeInfo.columnCnt == 1) {
      switch (typeInfo.componentBytes) {
      case 2:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::float16_t1;
        case 2: return ShaderVariableType::float16_t2;
        case 3: return ShaderVariableType::float16_t3;
        case 4: return ShaderVariableType::float16_t4;
        }
        break;
      case 4:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::float32_t1;
        case 2: return ShaderVariableType::float32_t2;
        case 3: return ShaderVariableType::float32_t3;
        case 4: return ShaderVariableType::float32_t4;
        }
        break;
      case 8:
        switch (typeInfo.vectorSize) {
        case 1: return ShaderVariableType::float64_t1;
        case 2: return ShaderVariableType::float64_t2;
        case 3: return ShaderVariableType::float64_t3;
        case 4: return ShaderVariableType::float64_t4;
        }
        break;
      }
    }
    // Matrix floats (columnCnt > 1)
    else {
      switch (typeInfo.componentBytes) {
      case 2:
        if (typeInfo.columnCnt == 2) {
          switch (typeInfo.vectorSize) {
          case 2: return ShaderVariableType::float16_t2x2;
          case 3: return ShaderVariableType::float16_t2x3;
          case 4: return ShaderVariableType::float16_t2x4;
          }
        } else if (typeInfo.columnCnt == 3) {
          switch (typeInfo.vectorSize) {
          case 2: return ShaderVariableType::float16_t3x2;
          case 3: return ShaderVariableType::float16_t3x3;
          case 4: return ShaderVariableType::float16_t3x4;
          }
        } else if (typeInfo.columnCnt == 4) {
          switch (typeInfo.vectorSize) {
          case 2: return ShaderVariableType::float16_t4x2;
          case 3: return ShaderVariableType::float16_t4x3;
          case 4: return ShaderVariableType::float16_t4x4;
          }
        }
        break;
      case 4:
        if (typeInfo.columnCnt == 2) {
          switch (typeInfo.vectorSize) {
          case 2: return ShaderVariableType::float32_t2x2;
          case 3: return ShaderVariableType::float32_t2x3;
          case 4: return ShaderVariableType::float32_t2x4;
          }
        } else if (typeInfo.columnCnt == 3) {
          switch (typeInfo.vectorSize) {
          case 2: return ShaderVariableType::float32_t3x2;
          case 3: return ShaderVariableType::float32_t3x3;
          case 4: return ShaderVariableType::float32_t3x4;
          }
        } else if (typeInfo.columnCnt == 4) {
          switch (typeInfo.vectorSize) {
          case 2: return ShaderVariableType::float32_t4x2;
          case 3: return ShaderVariableType::float32_t4x3;
          case 4: return ShaderVariableType::float32_t4x4;
          }
        }
        break;
      case 8:
        if (typeInfo.columnCnt == 2) {
          switch (typeInfo.vectorSize) {
          case 2: return ShaderVariableType::float64_t2x2;
          case 3: return ShaderVariableType::float64_t2x3;
          case 4: return ShaderVariableType::float64_t2x4;
          }
        } else if (typeInfo.columnCnt == 3) {
          switch (typeInfo.vectorSize) {
          case 2: return ShaderVariableType::float64_t3x2;
          case 3: return ShaderVariableType::float64_t3x3;
          case 4: return ShaderVariableType::float64_t3x4;
          }
        } else if (typeInfo.columnCnt == 4) {
          switch (typeInfo.vectorSize) {
          case 2: return ShaderVariableType::float64_t4x2;
          case 3: return ShaderVariableType::float64_t4x3;
          case 4: return ShaderVariableType::float64_t4x4;
          }
        }
        break;
      }
    }
  }

  // If we reach here, the type info doesn't match any known type
  return ShaderVariableType::Unknown;
}
// clang-format on

std::string ShaderVariableTypeInfo::toString() const {
  const std::string signPrefix = (signedness == ShaderVariableTypeInfo::Signedness::Unsigned) ? "u" : "";
  const std::string typeName = glzToString(baseType) | toLowerCase;
  const u8 bitCount = componentBytes * 8;
  const std::string matrixSuffix = columnCnt > 1 ? std::format("x{}", columnCnt) : "";
  return std::format("{}{}{}_t{}{}", signPrefix, typeName, bitCount, vectorSize, matrixSuffix);
}
std::string ShaderVariableTypeInfo::toMnemonicString() const {
  if (baseType == BaseType::Struct)
    return "struct";
  if (baseType == BaseType::Unknown)
    return "unknown";
  if (baseType == BaseType::Bool) {
    if (vectorSize == 1)
      return "bool";
    return "bvec" + std::to_string(vectorSize);
  }
  if (vectorSize == 1) {
    if (baseType == BaseType::Int)
      return std::string((signedness == Signedness::Unsigned ? "uint" : "int")) +
             (componentBytes == 8 ? "64_t" : "");
    if (baseType == BaseType::Float)
      return componentBytes == 8 ? "double" : "float";
  }

  if (baseType == BaseType::Float && columnCnt > 1) {
    std::string matType = componentBytes == 8 ? "dmat" : "mat";

    // Square matrices: mat2, mat3, mat4, dmat2, etc.
    if (vectorSize == columnCnt)
      return matType + std::to_string(vectorSize);
    // Non-square matrices: mat2x3, mat3x4, etc.
    else
      return matType + std::to_string(columnCnt) + "x" + std::to_string(vectorSize);
  }

  std::string result;
  if (baseType == BaseType::Int)
    result = (signedness == Signedness::Unsigned) ? "uvec" : "ivec";
  else if (baseType == BaseType::Float)
    result = (componentBytes == 8) ? "dvec" : "vec";
  return result + std::to_string(vectorSize);
}

CommonMemberProps getCommonMemberProps(const spirv_cross::Compiler& reflector,
                                       const spirv_cross::SPIRType& parentStructType, const u32 memberIx) {
  const spirv_cross::TypeID memberTypeId = parentStructType.member_types[memberIx];
  const spirv_cross::SPIRType& memberType = reflector.get_type(memberTypeId);
  const bool isArray = !memberType.array.empty();
  // The "base type" for an array is the type of its elements. For non-arrays, it's just the member_type.
  const auto& memberBaseType = isArray ? reflector.get_type(memberType.parent_type) : memberType;
  const u32 arrayStride =
      isArray ? reflector.get_member_decoration(parentStructType.self, memberIx, spv::DecorationArrayStride)
              : 0;

  return {
      .memberTypeId = memberTypeId,
      .memberType = memberType,
      .memberBaseType = memberBaseType,
      .typeInfo = ShaderVariableTypeInfo::fromSpirV(memberBaseType),
      .name = reflector.get_member_name(parentStructType.self, memberIx),
      .isArray = isArray,
      // We only support one-dimensional arrays at the moment, i.e., don't use array[1+]
      .arraySize = isArray ? memberType.array[0] : 0,
      .arrayStride = arrayStride,
  };
}

ShaderBlockMember parseStructMember(const spirv_cross::Compiler& reflector,
                                    const spirv_cross::SPIRType& parentStructType, const u32 memberIx) {
  const CommonMemberProps props = getCommonMemberProps(reflector, parentStructType, memberIx);
  ShaderBlockMember member{.typeInfo{props.typeInfo},
                           .name{props.name},
                           .isArray = props.isArray,
                           .arraySize = props.arraySize,
                           .arrayStride = props.arrayStride};

  if (member.typeInfo.baseType == ShaderVariableTypeInfo::BaseType::Struct)
    for (u32 subMemberIx = 0; subMemberIx < props.memberType.member_types.size(); ++subMemberIx)
      member.members.push_back(parseStructMember(reflector, props.memberBaseType, subMemberIx));

  member.offset = reflector.get_member_decoration(parentStructType.self, memberIx, spv::DecorationOffset);
  member.sizeBytes = reflector.get_declared_struct_member_size(parentStructType, memberIx);

  return member;
}

std::map<SetNo, std::map<BindingNo, ShaderResource>> reflectUniformBuffers(const SpirV& spirV) {
  spirv_cross::Compiler reflector(spirV);
  spirv_cross::ShaderResources resources = reflector.get_shader_resources();

  std::map<SetNo, std::map<BindingNo, ShaderResource>> uniformBuffers;
  for (const auto& resource : resources.uniform_buffers) {
    ShaderResource ubo;
    ubo.name = resource.name;
    ubo.set = reflector.get_decoration(resource.id, spv::DecorationDescriptorSet);
    ubo.binding = reflector.get_decoration(resource.id, spv::DecorationBinding);

    const auto& type = reflector.get_type(resource.base_type_id);
    ubo.sizeBytes = reflector.get_declared_struct_size(type);

    for (u32 memberIx = 0; memberIx < type.member_types.size(); ++memberIx)
      ubo.members.push_back(parseStructMember(reflector, type, memberIx));

    uniformBuffers[ubo.set][ubo.binding] = ubo;
  }

  return uniformBuffers;
}

ShaderInterfaceVariable parseIoBlockMember(const spirv_cross::Compiler& reflector,
                                           const spirv_cross::SPIRType& parentStructType, u32 memberIx) {
  const CommonMemberProps props = getCommonMemberProps(reflector, parentStructType, memberIx);
  ShaderInterfaceVariable var{.typeInfo{props.typeInfo},
                              .name{props.name},
                              .isArray = props.isArray,
                              .arraySize = props.arraySize,
                              .arrayStride = props.arrayStride};

  if (var.typeInfo.baseType == ShaderVariableTypeInfo::BaseType::Struct)
    for (uint32_t i = 0; i < props.memberBaseType.member_types.size(); ++i)
      var.members.push_back(parseIoBlockMember(reflector, props.memberBaseType, i));

  var.location = reflector.get_member_decoration(parentStructType.self, memberIx, spv::DecorationLocation);

  return var;
}

// Main function to reflect all stage inputs
std::map<LocationNo, ShaderInterfaceVariable>
reflectStageIO(const spirv_cross::Compiler& reflector,
               const spirv_cross::SmallVector<spirv_cross::Resource>& inputsOrOutputs) {
  std::map<LocationNo, ShaderInterfaceVariable> inputs;
  for (const auto& resource : inputsOrOutputs) {
    ShaderInterfaceVariable var;
    const auto& type = reflector.get_type(resource.base_type_id);

    var.name = resource.name;
    var.location = reflector.get_decoration(resource.id, spv::DecorationLocation);

    var.isArray = !type.array.empty();
    var.arraySize = var.isArray ? type.array[0] : 0;

    const auto& baseType = var.isArray ? reflector.get_type(type.parent_type) : type;
    var.typeInfo = ShaderVariableTypeInfo::fromSpirV(baseType);

    // If the top-level input is a struct (an "interface block"), parse its members
    if (baseType.basetype == spirv_cross::SPIRType::Struct)
      for (uint32_t i = 0; i < baseType.member_types.size(); ++i)
        var.members.push_back(parseIoBlockMember(reflector, baseType, i));

    inputs[var.location] = var;
  }

  return inputs;
}

ShaderStageSchema reflectShaderStageSchema(const SpirV& spirV) {
  ShaderStageSchema schema;

  const spirv_cross::Compiler reflector(spirV);
  auto resources = reflector.get_shader_resources();

  schema.inputs = reflectStageIO(reflector, resources.stage_inputs);
  schema.uniformsBuffers = reflectUniformBuffers(spirV);
  schema.outputs = reflectStageIO(reflector, resources.stage_outputs);
  return schema;
}

std::optional<ShaderResource> ShaderStageSchema::getMaterialUniformBufferSchema() const {

  const auto itSet = uniformsBuffers.find(asset::Material::kMaterialParamsSet);
  if (itSet == uniformsBuffers.end())
    // This material has no parameters
    return std::nullopt;

  const auto& bindings = itSet->second;
  const auto itBinding = bindings.find(asset::Material::kUniformParamsBinding);
  if (const bool hasUniformParams = itBinding != bindings.end(); !hasUniformParams)
    // This material has no uniforms
    return std::nullopt;

  const ShaderResource& uniformsSchema = itBinding->second;
  return uniformsSchema;
}

} // namespace aur::asset
