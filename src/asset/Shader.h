#pragma once

#include <filesystem>
#include <functional>

#include "../Handle.h"
#include "../VulkanWrappers.h"

namespace aur::asset {

struct ShaderStageDefinition {
  ShaderStage stage;
  std::vector<u32> spirv;
};

struct ShaderDefinition {
  ShaderStageDefinition vertStageDef;
  ShaderStageDefinition fragStageDef;
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
  [[nodiscard]] const std::vector<u32>& getFragmentBlob() const { return def_.fragStageDef.spirv; }
  [[nodiscard]] const std::string& getDebugName() const { return debugName; }

private:
  Shader() = default;

  ShaderDefinition def_;
  std::string debugName;
};

using ShaderUpdateCallback = std::function<void(Handle<asset::Shader>)>;

} // namespace aur::asset
