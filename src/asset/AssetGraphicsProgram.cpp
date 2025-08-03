#include "GraphicsProgram.h"

namespace aur::asset {

GraphicsProgram GraphicsProgram::create(const GraphicsProgramDefinition& programDef,
                                        Handle<ShaderStage> vertStage, Handle<ShaderStage> fragStage) {

  GraphicsProgram program;
  program.def_ = programDef;
  // TODO(vug): later change with a reference to ShaderModule via asset handle
  program.vert_ = vertStage;
  program.frag_ = fragStage;

  return program;
}

} // namespace aur::asset