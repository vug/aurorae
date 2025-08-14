#include "ShaderStage.h"

#include <spirv_cross/spirv_reflect.hpp>

#include "../Logger.h"

namespace aur::asset {

ShaderStage ShaderStage::create(ShaderStageDefinition&& def) {
  ShaderStage stage;
  stage.stage_ = def.stage;
  stage.spirVBlob_ = std::move(def.spirv);
  return stage;
}

ShaderParameterType spirvTypeToShaderParameterType(const spirv_cross::SPIRType& type) {
  switch (type.basetype) {
  case spirv_cross::SPIRType::Float:
    if (type.columns > 1) { // It's a matrix
      if (type.columns == 3 && type.vecsize == 3)
        return ShaderParameterType::Mat3;
      if (type.columns == 4 && type.vecsize == 4)
        return ShaderParameterType::Mat4;
    } else { // It's a scalar or vector
      switch (type.vecsize) {
      case 1:
        return ShaderParameterType::Float;
      case 2:
        return ShaderParameterType::Vec2;
      case 3:
        return ShaderParameterType::Vec3;
      case 4:
        return ShaderParameterType::Vec4;
      default:
        log().fatal("Incorrect vecsize");
      }
    }
    break;

  case spirv_cross::SPIRType::Int:
    // Assuming no int matrices, only vectors
    switch (type.vecsize) {
    case 1:
      return ShaderParameterType::Int;
    case 2:
      return ShaderParameterType::IVec2;
    case 3:
      return ShaderParameterType::IVec3;
    case 4:
      return ShaderParameterType::IVec4;
    }
    break;

  case spirv_cross::SPIRType::Boolean:
    // Bools are often represented as ints in uniform blocks,
    // but SPIRV-Cross can identify them.
    if (type.vecsize == 1)
      return ShaderParameterType::Bool;
    // Add bvec2, etc. if needed
    break;

  case spirv_cross::SPIRType::Struct:
    return ShaderParameterType::Struct;

  default:
    return ShaderParameterType::Unknown;
  }
  return ShaderParameterType::Unknown;
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

// A structure to hold the relevant information about a single variable
// in the interface between shader stages (e.g., a `vec3` at `location = 0`).
struct ShaderInterfaceVariable {
  uint32_t location; // layout(location = N)
  spirv_cross::SPIRType::BaseType type;
  uint32_t vecsize;
  uint32_t columns;
  std::string name;

  // We primarily sort by location to ensure a consistent order for comparison.
  bool operator<(const ShaderInterfaceVariable& other) const { return location < other.location; }

  // For validation, we require location, type, and dimensions to be identical.
  // The name is not used for comparison as it can be optimized away or differ,
  // but it's useful for logging and debugging.
  bool operator==(const ShaderInterfaceVariable& other) const {
    return location == other.location && type == other.type && vecsize == other.vecsize &&
           columns == other.columns;
  }
};

std::string basetype_to_string(spirv_cross::SPIRType::BaseType type) {
  using BaseType = spirv_cross::SPIRType::BaseType;
  switch (type) {
  case BaseType::Unknown:
    return "Unknown";
  case BaseType::Void:
    return "Void";
  case BaseType::Boolean:
    return "Boolean";
  case BaseType::Char:
    return "Char";
  case BaseType::Int:
    return "Int";
  case BaseType::UInt:
    return "UInt";
  case BaseType::Int64:
    return "Int64";
  case BaseType::UInt64:
    return "UInt64";
  case BaseType::Half:
    return "Half";
  case BaseType::Float:
    return "Float";
  case BaseType::Double:
    return "Double";
  case BaseType::Struct:
    return "Struct";
  case BaseType::Image:
    return "Image";
  case BaseType::SampledImage:
    return "SampledImage";
  case BaseType::Sampler:
    return "Sampler";
  case BaseType::AccelerationStructure:
    return "AccelerationStructure";
  default:
    return "Other";
  }
}

static std::vector<ShaderInterfaceVariable>
extractInterfaceVariables(const spirv_cross::Compiler& compiler,
                          const spirv_cross::SmallVector<spirv_cross::Resource>& resources) {

  std::vector<ShaderInterfaceVariable> variables;

  for (const auto& resource : resources) {
    // We must ignore built-in variables like gl_Position, gl_FragCoord, etc.,
    // as they are part of the system interface, not the user-defined one.
    if (compiler.has_decoration(resource.id, spv::DecorationBuiltIn)) {
      continue;
    }

    const auto& type = compiler.get_type(resource.type_id);

    variables.push_back({.location = compiler.get_decoration(resource.id, spv::DecorationLocation),
                         .type = type.basetype,
                         .vecsize = type.vecsize,
                         .columns = type.columns,
                         .name = resource.name});
  }

  // Sort the variables by location. This provides a canonical order, making
  // the comparison between vertex outputs and fragment inputs straightforward.
  std::sort(variables.begin(), variables.end());

  return variables;
}

// A helper function to print the details of an interface variable.
static void printVariable(const ShaderInterfaceVariable& var) {
  log().info("  - Location: {}, Type: {}, , VecSize: {}, Columns: {}, Name: {}", var.location,
             basetype_to_string(var.type), var.vecsize, var.columns, var.name);
}

bool validate_shader_linkage(const std::vector<uint32_t>& vertex_spirv,
                             const std::vector<uint32_t>& fragment_spirv) {

  // 1. Create compiler instances for both shader stages.
  spirv_cross::Compiler vert_compiler(vertex_spirv);
  spirv_cross::Compiler frag_compiler(fragment_spirv);

  // 2. Extract the shader resources (lists of inputs, outputs, uniforms, etc.).
  spirv_cross::ShaderResources vert_resources = vert_compiler.get_shader_resources();
  spirv_cross::ShaderResources frag_resources = frag_compiler.get_shader_resources();

  // 3. Extract the interface variables we care about:
  //    - For the Vertex Shader, we get its STAGE OUTPUTS.
  //    - For the Fragment Shader, we get its STAGE INPUTS.
  std::vector<ShaderInterfaceVariable> vert_outputs =
      extractInterfaceVariables(vert_compiler, vert_resources.stage_outputs);

  std::vector<ShaderInterfaceVariable> frag_inputs =
      extractInterfaceVariables(frag_compiler, frag_resources.stage_inputs);

  // 4. Print the results for inspection.
  log().info("Vertex Shader Outputs");
  for (const auto& var : vert_outputs)
    printVariable(var);

  log().info("Fragment Shader Inputs");
  for (const auto& var : frag_inputs)
    printVariable(var);

  // 5. The validation step:
  // Because we sorted the variables by location, we can simply compare the vectors.
  // If they are identical, the interface contract is met.
  if (vert_outputs == frag_inputs) {
    log().info("[SUCCESS] Vertex and Fragment shader interfaces match.");
    return true;
  } else {
    log().info("[FAILURE] Mismatch between Vertex outputs and Fragment inputs!");
    return false;
  }
}

constexpr u32 kSpirVMagic = 0x07230203; // SPIR-V magic number
std::string getSpirvVersionString(u32 version) {
  const u32 major = (version >> 16) & 0xFFFF; // Upper 16 bits
  const u32 minor = (version >> 8) & 0xFF;    // Next 8 bits
  return std::to_string(major) + "." + std::to_string(minor);
}
std::string getSpirvGeneratorString(uint32_t generator) {
  const u16 vendorID = static_cast<u16>(generator >> 16);
  const u16 toolVersion = static_cast<u16>(generator & 0xFFFF);

  // Map Vendor ID to human-readable names from https://registry.khronos.org/SPIR-V/api/spir-v.xml
  std::string vendorName;
  // clang-format off
  switch (vendorID) {
  case 0: vendorName = "Khronos"; break;
  case 1: vendorName = "LunarG"; break;
  case 2: vendorName = "Valve"; break;
  case 3: vendorName = "Codeplay"; break;
  case 4: vendorName = "NVIDIA"; break;
  case 5: vendorName = "ARM"; break;
  case 6: vendorName = "Khronos - LLVM Generator"; break;
  case 7: vendorName = "Khronos - Assembler"; break;
  case 8: vendorName = "Khronos - Glslang"; break;
  case 17: vendorName = "Khronos - Linker"; break;
  default: vendorName = "Unknown Vendor"; break;
  }
  // clang-format on
  return vendorName + " (Version " + std::to_string(toolVersion) + ")";
}

ShaderParameterSchema ShaderStage::getSchema(const SpirV& spirV) {
  const spirv_cross::Compiler reflector(spirV);
  auto resources = reflector.get_shader_resources();
  log().debug("Vertex Inputs:");
  for (const auto& input : resources.stage_inputs) {
    const spirv_cross::SPIRType& type = reflector.get_type(input.base_type_id);
    const uint32_t location = reflector.get_decoration(input.id, spv::DecorationLocation);
    log().info("Found input: '{} {}' at location {}", spirVBaseTypeToString(type), input.name.c_str(),
               location);

    // spv::DecorationComponent for multiple render targets
    if (reflector.has_decoration(input.id, spv::DecorationFlat)) // NoPerspective, Centroid
      log().info("  - Has 'flat' interpolation qualifier.");

    if (type.basetype == spirv_cross::SPIRType::Struct) {
      // For "in VertexOutput v", this gives "VertexOutput"
      const std::string& structName = reflector.get_name(input.base_type_id);
      log().info("  - It's a struct of type: '{}'", structName.c_str());
      log().info("  - Members:");

      // Iterate over the members of the struct
      for (uint32_t i = 0; i < type.member_types.size(); ++i) {
        const spirv_cross::TypedID memberTypeId = type.member_types[i];
        const spirv_cross::SPIRType memberType = reflector.get_type(memberTypeId);

        const std::string& memberName = reflector.get_member_name(input.base_type_id, i);
        log().info("    - {}[{},{}] {}", spirVBaseTypeToString(memberType), memberType.vecsize,
                   memberType.columns, memberName.c_str());
      }
    }
  }

  ShaderParameterSchema schema;
  for (const spirv_cross::Resource& uniform : resources.uniform_buffers) {
    const u32 set = reflector.get_decoration(uniform.id, spv::DecorationDescriptorSet);
    const u32 binding = reflector.get_decoration(uniform.id, spv::DecorationBinding);

    if (set != 1 || binding != 0)
      continue;

    const std::string_view blockVariableName = uniform.name;
    const std::string_view structName = reflector.get_name(uniform.base_type_id);

    const spirv_cross::SPIRType& blockType = reflector.get_type(uniform.base_type_id);
    const u64 bufferSize = reflector.get_declared_struct_size(blockType);

    for (uint32_t i = 0; i < blockType.member_types.size(); ++i) {
      const auto& memberTypeId = blockType.member_types[i];
      const auto& memberType = reflector.get_type(memberTypeId);

      ShaderParameter param;

      param.name = reflector.get_member_name(uniform.type_id, i); // "vizMode"
      auto foo = reflector.get_member_name(blockType.self, i);

      param.type = spirvTypeToShaderParameterType(reflector.get_type(blockType.member_types[i]));
      param.offset = reflector.type_struct_member_offset(blockType, i);
      size_t member_offset = reflector.get_member_decoration(blockType.self, i, spv::DecorationOffset);
      param.sizeBytes = reflector.get_declared_struct_member_size(blockType, i);
      param.binding = binding;

      // param.type = spirvTypeToParameterType(memberType);
      param.isArray = !memberType.array.empty();
      if (param.isArray)
        param.arraySize = memberType.array[0];

      schema.uniformBufferParams.push_back(param);
    }

    // Log all the names we found
    log().info("Found uniform block:");
    log().info("  Variable name: '{}'", blockVariableName); // "matParams"
    log().info("  Struct name: '{}'", structName);          // "MaterialParams"
    log().info("  Members:");
    for (const auto& param : schema.uniformBufferParams) {
      log().info("    - {}", param.name); // "vizMode"
    }
  }

  return schema;
}

bool ShaderStage::validateSpirV(const std::vector<u32>& blob) {
  // A SPIR-V should have at least the first 5 words (magic, version, generator, bound, schema)
  if (blob.size() < 5)
    return false;

  const u32* words = blob.data();

  if (words[0] != kSpirVMagic)
    return false;

  const u32 version = words[1];
  const u32 generator = words[2];
  const u32 bound = words[3];
  if (const u32 schema = words[4]; schema != 0) {
    log().warn("SPIR-V schema has to be 0, but is {}.", schema);
    return false;
  }
  log().debug("SPIR-V version: {}, generator: {}, bound: {}", getSpirvVersionString(version),
              getSpirvGeneratorString(generator), bound);

  return true;
}

} // namespace aur::asset