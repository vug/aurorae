#include "GraphicsProgram.h"

#include "../Logger.h"

namespace aur::asset {

ShaderStageSchema GraphicsProgramDefinition::combineSchemas(const ShaderStageSchema& vertSchema,
                                                            const ShaderStageSchema& fragSchema) {
  ShaderStageSchema combined = vertSchema;
  auto& ubos = combined.uniformsBuffers;
  const auto& fragUbos = fragSchema.uniformsBuffers;
  for (const auto& [setNo, bindings] : fragUbos) {
    for (const auto& [bindingNo, fragResource] : bindings) {
      // Create the slot for that set in combined UBOs if it doesn't exist
      auto& targetSet = ubos[setNo];
      const auto bindingIt = targetSet.find(bindingNo);
      // Resource doesn't exist in vertex shader. copy it.
      if (bindingIt == targetSet.end()) {
        targetSet[bindingNo] = fragResource;
        continue;
      }

      // Resource exists - validate it
      if (bindingIt->second != fragResource)
        log().fatal("Shader resource mismatch at set {} binding {}: vertex '{}' vs fragment '{}'", setNo,
                    bindingNo, bindingIt->second.name, fragResource.name);
    }
  }

  // Check that fragment shader inputs are a subset of vertex shader outputs
  for (const auto& [loc, fragInput] : fragSchema.inputs) {
    const auto vertOutputIt = vertSchema.outputs.find(loc);
    if (vertOutputIt == vertSchema.outputs.end()) {
      log().fatal("Fragment shader input at location {} ('{}') has no corresponding vertex shader output",
                  loc, fragInput.name);
    }
    if (vertOutputIt->second != fragInput) {
      log().fatal("Vertex/Fragment shader interface mismatch at location {}: vertex output '{}' vs fragment input '{}'",
                  loc, vertOutputIt->second.name, fragInput.name);
    }
  }

  return combined;
}

GraphicsProgram GraphicsProgram::create(GraphicsProgramDefinition&& graphicsProgramDef,
                                        Handle<ShaderStage> vertStage, Handle<ShaderStage> fragStage) {
  GraphicsProgram program;
  program.vert_ = vertStage;
  program.frag_ = fragStage;
  program.combinedSchema_ = std::move(graphicsProgramDef.combinedSchema);

  return program;
}
} // namespace aur::asset