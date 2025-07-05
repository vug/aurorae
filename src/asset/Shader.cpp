#include "Shader.h"

namespace aur::asset {

Shader Shader::create(const ShaderDefinition& def) {

  Shader shader;
  shader.def_ = def;

  return shader;
}

} // namespace aur::asset