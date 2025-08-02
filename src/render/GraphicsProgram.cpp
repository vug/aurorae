#include "GraphicsProgram.h"

#include "../Renderer.h"

namespace aur::render {

GraphicsProgram::GraphicsProgram(const Renderer& renderer, Handle<asset::GraphicsProgram> asset)
    : renderer_{&renderer}
    , assetHandle_{asset}
    , vertModule_{[this]() {
      const asset::GraphicsProgram& aGraphicsProgram = assetHandle_.get();
      const ShaderModuleCreateInfo vertCreateInfo{.spirv = &aGraphicsProgram.getVertexBlob()};
      return renderer_->createShaderModule(vertCreateInfo, aGraphicsProgram.getDebugName() + " Module");
    }()}
    , fragModule_{[this]() {
      const asset::GraphicsProgram& aGraphicsProgram = assetHandle_.get();
      const ShaderModuleCreateInfo fragCreateInfo{.spirv = &aGraphicsProgram.getFragmentBlob()};
      return renderer_->createShaderModule(fragCreateInfo, aGraphicsProgram.getDebugName() + " Module");
    }()} {}

} // namespace aur::render