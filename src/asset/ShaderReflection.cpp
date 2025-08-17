#include "ShaderReflection.h"

#include <spirv_cross/spirv_reflect.hpp>

#include "../Logger.h"
#include "ShaderStage.h"

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
  const std::string typeName = glz::write<glz::opts{.raw = true}>(baseType).value_or("ERROR");
  const u8 bitCount = componentBytes * 8;
  const std::string matrixSuffix = columnCnt > 1 ? std::format("x{}", columnCnt) : "";
  return std::format("{}{}{}_t{}{}", signPrefix, typeName, bitCount, vectorSize, matrixSuffix);
}

ShaderStageSchema reflectShaderStageSchema(const SpirV& spirV) {
  ShaderStageSchema schema;

  const spirv_cross::Compiler reflector(spirV);
  auto resources = reflector.get_shader_resources();
  for (const auto& input : resources.stage_inputs) {
    ShaderVariable var;

    const spirv_cross::SPIRType& inputType = reflector.get_type(input.base_type_id);
    var.typeInfo = ShaderVariableTypeInfo::fromSpirV(inputType);
    var.location = reflector.get_decoration(input.id, spv::DecorationLocation);
    // spv::DecorationComponent for multiple render targets
    // NoPerspective, Centroid
    var.isFlat = reflector.has_decoration(input.id, spv::DecorationFlat);

    if (inputType.basetype == spirv_cross::SPIRType::Struct) {
      // For "in VertexOutput v", this gives "VertexOutput"
      const std::string& structName = reflector.get_name(input.base_type_id);

      std::vector<ShaderVariable> members;
      std::string membersStr;
      for (uint32_t memberIx = 0; memberIx < inputType.member_types.size(); ++memberIx) {
        ShaderVariable& member = members.emplace_back();
        const spirv_cross::TypedID memberTypeId = inputType.member_types[memberIx];
        const spirv_cross::SPIRType memberType = reflector.get_type(memberTypeId);
        member.typeInfo = ShaderVariableTypeInfo::fromSpirV(memberType);
        member.name = reflector.get_member_name(input.base_type_id, memberIx);
        member.location = var.location + memberIx;

        membersStr += std::format("{} {} @{}; ", member.typeInfo.toString(), member.name, member.location);
      }
      // log().info("struct {} NAME {{ {} }}", structName, membersStr);
    }
  }

  for (const spirv_cross::Resource& uniform : resources.uniform_buffers) {
    const u32 setNo = reflector.get_decoration(uniform.id, spv::DecorationDescriptorSet);
    const u32 bindingNo = reflector.get_decoration(uniform.id, spv::DecorationBinding);

    auto& bindingSchemas = schema.descriptors.uniformsBuffers[setNo];
    UniformBufferSchema& uboSchema = bindingSchemas[bindingNo];
    uboSchema.name = uniform.name;
    // uboSchema.name = reflector.get_name(uniform.base_type_id);

    const spirv_cross::SPIRType& blockType = reflector.get_type(uniform.base_type_id);
    uboSchema.sizeBytes = reflector.get_declared_struct_size(blockType);

    const u64 memberCnt = blockType.member_types.size();
    uboSchema.variables.reserve(memberCnt);
    for (uint32_t memberIx = 0; memberIx < memberCnt; ++memberIx) {
      const auto& memberTypeId = blockType.member_types[memberIx];
      const auto& memberType = reflector.get_type(memberTypeId);

      const ShaderVariable var{
          .name = reflector.get_member_name(blockType.self, memberIx),
          .typeInfo = ShaderVariableTypeInfo::fromSpirV(memberType),
          .location = static_cast<u32>(-1),
          .set = setNo,
          .binding = bindingNo,
          // .offset = reflector.get_member_decoration(blockType.self, i, spv::DecorationOffset),
          .offset = reflector.type_struct_member_offset(blockType, memberIx),
          .sizeBytes = reflector.get_declared_struct_member_size(blockType, memberIx),
          .isArray = !memberType.array.empty(),
          .arraySize = memberType.array.empty() ? 1 : memberType.array[0],
      };
      uboSchema.variables.push_back(var);
    }
  }
  return schema;
}

} // namespace aur::asset
