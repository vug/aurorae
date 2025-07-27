#pragma once

#include <filesystem>
#include <functional>

#include "../Handle.h"
#include "../VulkanWrappers.h"
#include "Common.h"

namespace aur::asset {

using SpirV = std::vector<u32>;

struct ShaderStageDefinition {
  ShaderStage stage;
  std::vector<u32> spirv;
};

struct ShaderDefinition {
  // TODO(vug): replace with stableIds
  ShaderStageDefinition vertStageDef;
  // StableId<ShaderStageDefinition> vert;
  ShaderStageDefinition fragStageDef;
  // StableId<ShaderStageDefinition> frag;
};

class Shader {
public:
  static Shader create(const ShaderDefinition& shaderDef);

  ~Shader() = default;

  Shader(const Shader& other) = delete;
  Shader& operator=(const Shader& other) = delete;
  Shader(Shader&& other) noexcept = default;
  Shader& operator=(Shader&& other) noexcept = default;

  [[nodiscard]] const ShaderDefinition& getDefinition() const { return def_; }
  [[nodiscard]] const std::vector<u32>& getVertexBlob() const { return def_.vertStageDef.spirv; }
  // [[nodiscard]] const SpirV& getVertexBlob() const { return vertBlob; }
  [[nodiscard]] const std::vector<u32>& getFragmentBlob() const { return def_.fragStageDef.spirv; }
  // [[nodiscard]] const SpirV& getFragmentBlob() const { return fragBlob; }
  [[nodiscard]] const std::string& getDebugName() const { return debugName; }

private:
  Shader() = default;

  ShaderDefinition def_; // TODO(vug): remove
  SpirV vertBlob;
  SpirV fragBlob;
  std::string debugName;
};

using ShaderUpdateCallback = std::function<void(Handle<asset::Shader>)>;

} // namespace aur::asset
