#include "Shader.h"

#include "../FileIO.h"

namespace aur::asset {

std::optional<Shader> Shader::create(const ShaderDefinition& def) {
  if (!std::filesystem::exists(def.vertPath) || !std::filesystem::exists(def.fragPath))
    return {};

  Shader shader;
  shader.vertBlob = readBinaryFile(def.vertPath);
  shader.fragBlob = readBinaryFile(def.fragPath);
  // TODO(vug): is it possible to do some validation to ensure these are SPIR-V shader binary blobs?
  return shader;
}

Shader::Shader(Shader&& other) noexcept
    : vertBlob(std::exchange(other.vertBlob, {}))
    , fragBlob(std::exchange(other.fragBlob, {}))
    , debugName(std::exchange(other.debugName, {})) {}

Shader& Shader::operator=(Shader&& other) noexcept {
  if (this == &other)
    return *this;
  vertBlob = std::exchange(other.vertBlob, {});
  fragBlob = std::exchange(other.fragBlob, {});
  debugName = std::exchange(other.debugName, {});
  return *this;
}

} // namespace aur::asset