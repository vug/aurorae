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
  static Shader create(const ShaderDefinition& shaderDef, Handle<ShaderStage> vertStage,
                       Handle<ShaderStage> fragStage);

  ~Shader() = default;

  Shader(const Shader& other) = delete;
  Shader& operator=(const Shader& other) = delete;
  Shader(Shader&& other) noexcept = default;
  Shader& operator=(Shader&& other) noexcept = default;

  [[nodiscard]] const ShaderDefinition& getDefinition() const { return def_; }
  [[nodiscard]] const SpirV& getVertexBlob() const { return vert_.get().getSpirVBlob(); }
  [[nodiscard]] const SpirV& getFragmentBlob() const { return frag_.get().getSpirVBlob(); }
  [[nodiscard]] const std::string& getDebugName() const { return debugName_; }

private:
  Shader() = default;

  ShaderDefinition def_;
  Handle<ShaderStage> vert_;
  Handle<ShaderStage> frag_;
  std::string debugName_;
};

using ShaderUpdateCallback = std::function<void(Handle<asset::Shader>)>;

} // namespace aur::asset
