#include "GraphicsProgram.h"

namespace aur::asset {

GraphicsProgram GraphicsProgram::create(const GraphicsProgramDefinition& def, Handle<ShaderStage> vertStage,
                                        Handle<ShaderStage> fragStage) {

  GraphicsProgram shader;
  shader.def_ = def;
  // TODO(vug): later change with a reference to ShaderModule via asset handle
  shader.vert_ = vertStage;
  shader.frag_ = fragStage;

  return shader;
}

} // namespace aur::asset