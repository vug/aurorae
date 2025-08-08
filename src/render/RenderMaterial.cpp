#include "Material.h"

#include "../Pipeline.h"
#include "../Renderer.h"

namespace aur::render {

Material::Material(Renderer& renderer, Handle<asset::Material> asset)
    : renderer_{&renderer}
    , assetHandle_{asset}
    , graphicsProgramHandle_{[this]() {
      const asset::Material& aMaterial = assetHandle_.get();
      return renderer_->uploadOrGet(aMaterial.getGraphicsProgramHandle());
    }()}
    , pipelineCreateInfo_{[this]() {
      const asset::Material& aMaterial = assetHandle_.get();
      const asset::MaterialDefinition& matDef = aMaterial.getDefinition();
      PipelineCreateInfo info{.graphicsProgram = graphicsProgramHandle_,
                              .rasterizationState = {
                                  .polygonMode = matDef.polygonMode,
                                  .cullMode = matDef.cullMode,
                                  .frontFace = matDef.frontFace,
                                  .lineWidth = matDef.lineWidth,
                              }};
      return info;
    }()} {}

} // namespace aur::render