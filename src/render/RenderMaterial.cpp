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
                              .rasterizationState =
                                  {
                                      .polygonMode = matDef.polygonMode,
                                      .cullMode = matDef.cullMode,
                                      .frontFace = matDef.frontFace,
                                      .lineWidth = matDef.lineWidth,
                                  },
                              .depthStencilState =
                                  {
                                      .depthTest = matDef.depthTest,
                                      .depthWrite = matDef.depthWrite,
                                  },
                              .colorBlendState = colorBlendStateFromPreset(matDef.blendPreset)};
      return info;
    }()} {}

PipelineColorBlendStateCreateInfo Material::colorBlendStateFromPreset(BlendingPreset preset) {
  switch (preset) {
  case BlendingPreset::NoBlend: {
    PipelineColorBlendAttachmentState attachment{
        .enable = false,
    };
    PipelineColorBlendStateCreateInfo info;
    info.attachments = {attachment};
    return info;
  }
  case BlendingPreset::AlphaBlend: {
    PipelineColorBlendAttachmentState attachment{
        .enable = true,
        .srcColorFactor = BlendFactor::SrcAlpha,
        .dstColorFactor = BlendFactor::OneMinusSrcAlpha,
        .srcAlphaFactor = BlendFactor::One,
        .dstAlphaFactor = BlendFactor::Zero,
    };
    PipelineColorBlendStateCreateInfo info;
    info.attachments = {attachment};
    return info;
  }
  case BlendingPreset::Additive: {
    PipelineColorBlendAttachmentState attachment{
        .enable = true,
        .srcColorFactor = BlendFactor::One,
        .dstColorFactor = BlendFactor::One,
        .srcAlphaFactor = BlendFactor::One,
        .dstAlphaFactor = BlendFactor::One,
    };
    PipelineColorBlendStateCreateInfo info;
    info.attachments = {attachment};
    return info;
  }
  }
  std::unreachable();
}

} // namespace aur::render