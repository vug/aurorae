#include "Shader.h"

namespace aur::asset {

Shader Shader::create(const ShaderDefinition& def, const SpirV& vertSpirV, const SpirV& fragSpirV) {

  Shader shader;
  shader.def_ = def;
  // TODO(vug): later change with a reference to ShaderModule via asset handle
  shader.vertBlob_ = vertSpirV;
  shader.fragBlob_ = fragSpirV;

  return shader;
}

} // namespace aur::asset