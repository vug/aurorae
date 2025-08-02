#include "Shader.h"

#include "../Renderer.h"

namespace aur::render {

Shader::Shader(const Renderer& renderer, Handle<asset::GraphicsProgram> asset)
    : renderer_{&renderer}
    , assetHandle_{asset}
    , vertModule_{[this]() {
      const asset::GraphicsProgram& aShader = assetHandle_.get();
      const ShaderModuleCreateInfo vertCreateInfo{.spirv = &aShader.getVertexBlob()};
      return renderer_->createShaderModule(vertCreateInfo, aShader.getDebugName() + " Module");
    }()}
    , fragModule_{[this]() {
      const asset::GraphicsProgram& aShader = assetHandle_.get();
      const ShaderModuleCreateInfo fragCreateInfo{.spirv = &aShader.getFragmentBlob()};
      return renderer_->createShaderModule(fragCreateInfo, aShader.getDebugName() + " Module");
    }()} {}

} // namespace aur::render