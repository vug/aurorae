#include "GraphicsProgram.h"

#include "../Renderer.h"

namespace aur::render {

GraphicsProgram::GraphicsProgram(const Renderer& renderer, Handle<asset::GraphicsProgram> asset)
    : renderer_{&renderer}
    , assetHandle_{asset}
    , vertModule_{[this]() {
      const ShaderModuleCreateInfo vertCreateInfo{.spirv = &assetHandle_->getVertexBlob()};
      return renderer_->createShaderModule(vertCreateInfo, assetHandle_->getDebugName() + " Module");
    }()}
    , fragModule_{[this]() {
      const ShaderModuleCreateInfo fragCreateInfo{.spirv = &assetHandle_->getFragmentBlob()};
      return renderer_->createShaderModule(fragCreateInfo, assetHandle_->getDebugName() + " Module");
    }()} {}

} // namespace aur::render