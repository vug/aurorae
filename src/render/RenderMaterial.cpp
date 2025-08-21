#include "Material.h"

#include "../Pipeline.h"
#include "../Renderer.h"

namespace aur::render {

Material::Material(Renderer& renderer, Handle<asset::Material> asset)
    : renderer_{&renderer}
    , assetHandle_{asset}
    , graphicsProgramHandle_{[this]() {
      return renderer_->uploadOrGet(assetHandle_->getGraphicsProgramHandle());
    }()}
    , pipelineCreateInfo_{[this]() {
      const asset::MaterialDefinition& matDef = assetHandle_->getDefinition();
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
    }()} {
  const std::map<asset::SetNo, std::map<asset::BindingNo, asset::ShaderResource>>& ubos =
      assetHandle_->getGraphicsProgramHandle()->getCombinedSchema().uniformsBuffers;
  constexpr u32 kMatParamSet = 0;
  constexpr u32 kMatParamBinding = 0;
  const auto itSet = ubos.find(kMatParamSet);
  if (itSet == ubos.end())
    return;
  const auto& bindings = itSet->second;
  const auto itBinding = bindings.find(kMatParamBinding);
  if (itBinding == bindings.end())
    return;
  const asset::ShaderResource& matParamUboSchema = itBinding->second;

  const BufferCreateInfo createInfo{.sizeBytes = matParamUboSchema.sizeBytes,
                                    .usages = {BufferUsage::Uniform},
                                    .memoryUsage = MemoryUsage::CpuToGpu};
  matParamsUbo_ = renderer_->createBuffer(createInfo, "MatParam Uniform Buffer");
}

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