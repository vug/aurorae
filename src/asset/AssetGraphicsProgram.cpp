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

  if (vertSchema.outputs != fragSchema.inputs)
    log().fatal("Vertex shader outputs don't match fragment shader inputs");

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