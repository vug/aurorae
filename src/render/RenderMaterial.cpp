#include "Material.h"

#include "../Logger.h"
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
  constexpr u32 kMatParamSet = 1;
  constexpr u32 kMatParamBinding = 0;
  const auto itSet = ubos.find(kMatParamSet);
  if (itSet == ubos.end())
    return;
  const auto& bindings = itSet->second;
  const auto itBinding = bindings.find(kMatParamBinding);
  if (itBinding == bindings.end())
    return;
  matParamUboSchema_ = itBinding->second;

  {
    const BufferCreateInfo createInfo{.sizeBytes = matParamUboSchema_.sizeBytes,
                                      .usages = {BufferUsage::Uniform},
                                      .memoryUsage = MemoryUsage::CpuToGpu};
    matParamsUbo_ = renderer_->createBuffer(createInfo, "MatParam Uniform Buffer");
  }

  {
    const DescriptorSetCreateInfo createInfo{.layout =
                                                 graphicsProgramHandle_->getDescriptorSetLayoutRefs()[1]};
    matParamsDescriptorSet_ = renderer_->createDescriptorSet(createInfo, "MatParam DescriptorSet");
  }
}
void Material::setParam(std::string_view name, std::span<const std::byte> data) const {
  for (const asset::ShaderBlockMember& member : matParamUboSchema_.members) {
    if (member.name == name) {
      if (data.size_bytes() != member.sizeBytes)
        log().fatal("incorrect size of data for material parameter '{}'", name);

      std::byte* dst = (std::byte*)matParamsUbo_.map();
      memcpy(dst + member.offset, data.data(), data.size_bytes());
      matParamsUbo_.unmap();

      // TODO(vug): iterate over members, create a map from param name to BufferInfo
      DescriptorBufferInfo bufferInfo{
          .buffer = &matParamsUbo_,
          .offset = member.offset,
          .range = member.sizeBytes,
      };

      const WriteDescriptorSet write{
          .dstSet = &matParamsDescriptorSet_,
          .binding = 0,
          .descriptorCnt = 1,
          .descriptorType = DescriptorType::UniformBuffer,
          .bufferInfo = &bufferInfo,
      };
      matParamsDescriptorSet_.update({write});

      return;
    }
  }
  log().fatal("material parameter '{}' does not found", name);
}

PipelineColorBlendStateCreateInfo Material::colorBlendStateFromPreset(BlendingPreset preset) {
  switch (preset) {
  case BlendingPreset::NoBlend: {
    const PipelineColorBlendAttachmentState attachment{
        .enable = false,
    };
    PipelineColorBlendStateCreateInfo info;
    info.attachments = {attachment};
    return info;
  }
  case BlendingPreset::AlphaBlend: {
    const PipelineColorBlendAttachmentState attachment{
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
    const PipelineColorBlendAttachmentState attachment{
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