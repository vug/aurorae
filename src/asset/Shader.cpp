#include "Shader.h"

#include "../FileIO.h"

namespace aur::asset {

std::optional<Shader> Shader::create(const ShaderDefinition& def) {

  Shader shader;
  shader.def_ = def;

  return shader;
}

Shader::Shader(Shader&& other) noexcept
    : def_(std::exchange(other.def_, {}))
    , debugName(std::exchange(other.debugName, {})) {}

Shader& Shader::operator=(Shader&& other) noexcept {
  if (this == &other)
    return *this;
  def_ = std::exchange(other.def_, {});
  debugName = std::exchange(other.debugName, {});
  return *this;
}

} // namespace aur::asset