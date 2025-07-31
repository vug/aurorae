#pragma once

#include <filesystem>
#include <functional>

#include "../Handle.h"
#include "ShaderStage.h"

#include "Common.h"

namespace aur::asset {

struct ShaderDefinition {
  StableId<ShaderStageDefinition> vert;
  StableId<ShaderStageDefinition> frag;
};

class Shader {
public:
  // TODO(vug): instead of copying SpirV blob, make ShaderStage an asset and refer to them
  static Shader create(const ShaderDefinition& shaderDef, const SpirV& vertSpirV, const SpirV& fragSpirV);

  ~Shader() = default;

  Shader(const Shader& other) = delete;
  Shader& operator=(const Shader& other) = delete;
  Shader(Shader&& other) noexcept = default;
  Shader& operator=(Shader&& other) noexcept = default;

  [[nodiscard]] const ShaderDefinition& getDefinition() const { return def_; }
  [[nodiscard]] const SpirV& getVertexBlob() const { return vertBlob_; }
  [[nodiscard]] const SpirV& getFragmentBlob() const { return fragBlob_; }
  [[nodiscard]] const std::string& getDebugName() const { return debugName_; }

private:
  Shader() = default;

  ShaderDefinition def_;
  SpirV vertBlob_;
  SpirV fragBlob_;
  std::string debugName_;
};

using ShaderUpdateCallback = std::function<void(Handle<asset::Shader>)>;

} // namespace aur::asset
