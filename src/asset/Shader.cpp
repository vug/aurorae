#include "Shader.h"

namespace aur::asset {

Shader Shader::create(const ShaderDefinition& def, Handle<ShaderStage> vertStage,
                      Handle<ShaderStage> fragStage) {

  Shader shader;
  shader.def_ = def;
  // TODO(vug): later change with a reference to ShaderModule via asset handle
  shader.vert_ = vertStage;
  shader.frag_ = fragStage;

  return shader;
}

} // namespace aur::asset