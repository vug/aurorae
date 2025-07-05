#include "Shader.h"

#include "../Renderer.h"

namespace aur::render {

Shader::Shader(const Renderer& renderer, Handle<asset::Shader> asset)
    : renderer_{&renderer}
    , assetHandle_{asset}
    , vertModule_{[this]() {
      const asset::Shader& aShader = assetHandle_.get();
      const ShaderModuleCreateInfo vertCreateInfo{.codeBlob = &aShader.getVertexBlob()};
      return renderer_->createShaderModule(vertCreateInfo, aShader.getDebugName() + " Module");
    }()}
    , fragModule_{[this]() {
      const asset::Shader& aShader = assetHandle_.get();
      const ShaderModuleCreateInfo fragCreateInfo{.codeBlob = &aShader.getFragmentBlob()};
      return renderer_->createShaderModule(fragCreateInfo, aShader.getDebugName() + " Module");
    }()} {}

} // namespace aur::render