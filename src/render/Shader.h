#pragma once

#include "../Handle.h"
#include "../Resources/ShaderModule.h"
#include "../asset/GraphicsProgram.h"

namespace aur {
class Renderer;
}

namespace aur::render {
class Shader {
public:
  Shader() = default;
  Shader(const Renderer& renderer, Handle<asset::GraphicsProgram> asset);
  ~Shader() = default;

  Shader(const Shader& other) = delete;
  Shader(Shader&& other) noexcept = default;
  Shader& operator=(const Shader& other) = delete;
  Shader& operator=(Shader&& other) noexcept = default;

  [[nodiscard]] const Handle<asset::GraphicsProgram>& getAssetHandle() const { return assetHandle_; }
  [[nodiscard]] const ShaderModule& getVertexShaderModule() const { return vertModule_; }
  [[nodiscard]] const ShaderModule& getFragmentShaderModule() const { return fragModule_; }

private:
  const Renderer* renderer_{};
  Handle<asset::GraphicsProgram> assetHandle_;

  ShaderModule vertModule_;
  ShaderModule fragModule_;
};
} // namespace aur::render