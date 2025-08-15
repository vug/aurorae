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

ShaderParameterSchema ShaderStage::getSchema(const SpirV& spirV) {
  const spirv_cross::Compiler reflector(spirV);
  auto resources = reflector.get_shader_resources();
  log().debug("Vertex Inputs:");
  for (const auto& input : resources.stage_inputs) {
    ShaderVariable var;

    const spirv_cross::SPIRType& inputType = reflector.get_type(input.base_type_id);
    var.typeInfo = ShaderVariableTypeInfo::fromSpirV(inputType);
    var.location = reflector.get_decoration(input.id, spv::DecorationLocation);
    log().info("Found input: '?? {}' at location {}", input.name.c_str(), var.location);

    // spv::DecorationComponent for multiple render targets
    if (reflector.has_decoration(input.id, spv::DecorationFlat)) // NoPerspective, Centroid
      log().info("  - Has 'flat' interpolation qualifier.");

    if (inputType.basetype == spirv_cross::SPIRType::Struct) {
      // For "in VertexOutput v", this gives "VertexOutput"
      const std::string& structName = reflector.get_name(input.base_type_id);
      log().info("  - It's a struct of type: '{}'", structName.c_str());
      log().info("  - Members:");

      // Iterate over the members of the struct
      for (uint32_t i = 0; i < inputType.member_types.size(); ++i) {
        const spirv_cross::TypedID memberTypeId = inputType.member_types[i];
        const spirv_cross::SPIRType memberType = reflector.get_type(memberTypeId);

        const std::string& memberName = reflector.get_member_name(input.base_type_id, i);
        log().info("    - ??[{},{}] {}", memberType.vecsize, memberType.columns, memberName.c_str());
      }
    }
  }

  ShaderParameterSchema schema;
  for (const spirv_cross::Resource& uniform : resources.uniform_buffers) {
    const u32 set = reflector.get_decoration(uniform.id, spv::DecorationDescriptorSet);
    const u32 binding = reflector.get_decoration(uniform.id, spv::DecorationBinding);

    // if (set != 1 || binding != 0)
    //   continue;

    const std::string_view blockVariableName = uniform.name;
    // const std::string_view structName = reflector.get_name(uniform.base_type_id);

    const spirv_cross::SPIRType& blockType = reflector.get_type(uniform.base_type_id);
    const u64 bufferSize = reflector.get_declared_struct_size(blockType);

    for (uint32_t i = 0; i < blockType.member_types.size(); ++i) {
      const auto& memberTypeId = blockType.member_types[i];
      const auto& memberType = reflector.get_type(memberTypeId);

      const ShaderVariable var{
          .name = reflector.get_member_name(blockType.self, i),
          .category = ShaderVariableCategory::UniformBufferMember,
          .typeInfo = ShaderVariableTypeInfo::fromSpirV(memberType),
          .location = static_cast<u32>(-1),
          .set = set,
          .binding = binding,
          // .offset = reflector.get_member_decoration(blockType.self, i, spv::DecorationOffset),
          .offset = reflector.type_struct_member_offset(blockType, i),
          .sizeBytes = reflector.get_declared_struct_member_size(blockType, i),
          .isArray = !memberType.array.empty(),
          .arraySize = memberType.array.empty() ? 1 : memberType.array[0],
      };

      schema.uniformBufferParams.push_back(var);
    }

    // Log all the names we found
    // "matParams", "MaterialParams"
    std::string paramsStr;
    for (const auto& param : schema.uniformBufferParams)
      paramsStr += std::format("    {} {};\n", param.typeInfo.toString(), param.name); // "vizMode"
    log().info("Found uniform block:\n  struct {} {{\n{}  }}\n  of size {} bytes", blockVariableName,
               paramsStr, bufferSize);
  }
  return schema;
}

// VERTEX SHADER <-> FRAGMENT SHADER COMPATIBILITY -> MODE TO GRAPHICS PROGRAM

/*
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
             spirVBaseTypeToString(var.type), var.vecsize, var.columns, var.name);
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

*/

// VALIDATION

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