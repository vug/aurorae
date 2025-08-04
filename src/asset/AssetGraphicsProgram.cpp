#include "GraphicsProgram.h"

namespace aur::asset {

GraphicsProgram GraphicsProgram::create(Handle<ShaderStage> vertStage, Handle<ShaderStage> fragStage) {

  GraphicsProgram program;
  program.vert_ = vertStage;
  program.frag_ = fragStage;

  return program;
}

} // namespace aur::asset