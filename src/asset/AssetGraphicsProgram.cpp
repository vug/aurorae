#include "GraphicsProgram.h"

#include "../Logger.h"

namespace aur::asset {

GraphicsProgram GraphicsProgram::create(Handle<ShaderStage> vertStage, Handle<ShaderStage> fragStage) {
  GraphicsProgram program;
  program.vert_ = vertStage;
  program.frag_ = fragStage;

  program.combinedSchema_ = vertStage->getSchema();
  auto& ubos = program.combinedSchema_.uniformsBuffers;
  const auto& fragUbos = fragStage->getSchema().uniformsBuffers;
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

  if (vertStage->getSchema().outputs != fragStage->getSchema().inputs)
    log().fatal("Vertex shader outputs don't match fragment shader inputs");

  return program;
}
} // namespace aur::asset