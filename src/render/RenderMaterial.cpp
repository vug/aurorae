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
  const auto itSet = ubos.find(kMatParamSet);
  if (itSet == ubos.end())
    // This material has no parameters
    return;

  const auto& bindings = itSet->second;
  const auto itBinding = bindings.find(kUniformParamsBinding);
  if (const bool hasUniformParams = itBinding != bindings.end(); !hasUniformParams)
    return;

  {
    matParamUboSchema_ = itBinding->second;
    const BufferCreateInfo createInfo{.sizeBytes = matParamUboSchema_.sizeBytes,
                                      .usages = {BufferUsage::Uniform},
                                      .memoryUsage = MemoryUsage::CpuToGpu};
    matUniformsUbo_ = renderer_->createBuffer(createInfo, "Material Uniforms Buffer");
    matUniformsUbo_.map();
  }

  // Will create other resources here

  {
    const DescriptorSetCreateInfo createInfo{.layout =
                                                 graphicsProgramHandle_->getDescriptorSetLayoutRefs()[1]};
    matParamsDescriptorSet_ = renderer_->createDescriptorSet(createInfo, "MatParams DescriptorSet");
  }

  // Set / Update all created resources to this descriptor set
  {
    // TODO(vug): iterate over members, create a map from param name to BufferInfo
    DescriptorBufferInfo bufferInfo{
        .buffer = &matUniformsUbo_,
        .offset = 0,
        .range = matUniformsUbo_.getCreateInfo().sizeBytes,
    };

    const WriteDescriptorSet write{
        .binding = kUniformParamsBinding,
        .descriptorCnt = 1,
        .descriptorType = DescriptorType::UniformBuffer,
        .bufferInfo = &bufferInfo,
    };
    matParamsDescriptorSet_.update({write});
  }

  matVals_ = buildDefaultValues(matParamUboSchema_.members);
}

Material::~Material() {
  matUniformsUbo_.unmap();
}

void Material::setParam(std::string_view name, std::span<const std::byte> data) const {
  for (const asset::ShaderBlockMember& member : matParamUboSchema_.members) {
    if (member.name == name) {
      // TODO(vug): introduce more & better checks
      if (data.size_bytes() != member.sizeBytes)
        log().fatal("incorrect size of data for material parameter '{}'", name);

      memcpy(matUniformsUbo_.getMapPtr() + member.offset, data.data(), data.size_bytes());
      return;
    }
  }
  log().fatal("material parameter '{}' does not found", name);
}
MaterialUniformValue::Struct
Material::buildDefaultValues(const std::vector<asset::ShaderBlockMember>& members) {
  auto createDefaultValue = [](const asset::ShaderVariableTypeInfo& typeInfo)
      -> std::variant<i32, u32, f32, glm::vec2, glm::vec3, glm::vec4, glm::mat3, glm::mat4,
                      MaterialUniformValue::Struct, MaterialUniformValue::Array> {
    using BaseType = asset::ShaderVariableTypeInfo::BaseType;
    using Signedness = asset::ShaderVariableTypeInfo::Signedness;

    if (typeInfo.baseType == BaseType::Struct) {
      return MaterialUniformValue::Struct{};
    }

    if (typeInfo.baseType == BaseType::Float && typeInfo.componentBytes == 4) {
      if (typeInfo.columnCnt > 1) {
        // Matrix types
        if (typeInfo.vectorSize == 3 && typeInfo.columnCnt == 3) {
          return glm::mat3{};
        } else if (typeInfo.vectorSize == 4 && typeInfo.columnCnt == 4) {
          return glm::mat4{};
        }
      } else {
        // Vector/scalar types
        if (typeInfo.vectorSize == 1) {
          return f32{};
        } else if (typeInfo.vectorSize == 2) {
          return glm::vec2{};
        } else if (typeInfo.vectorSize == 3) {
          return glm::vec3{};
        } else if (typeInfo.vectorSize == 4) {
          return glm::vec4{};
        }
      }
    }
    if (typeInfo.baseType == BaseType::Int && typeInfo.componentBytes == 4) {
      if (typeInfo.signedness == Signedness::Signed) {
        return i32{};
      } else if (typeInfo.signedness == Signedness::Unsigned) {
        return u32{};
      }
    }

    // Default fallback
    return i32{};
  };

  auto recurse = [&createDefaultValue](this auto&& self, const asset::ShaderBlockMember& var,
                                       MaterialUniformValue& matVal) -> void {
    if (var.typeInfo.baseType == asset::ShaderVariableTypeInfo::BaseType::Struct) {
      matVal.val = MaterialUniformValue::Struct{};
      auto& structData = std::get<MaterialUniformValue::Struct>(matVal.val);
      for (const asset::ShaderBlockMember& child : var.members) {
        MaterialUniformValue& childVal = structData[child.name];
        self(child, childVal);
      }
    } else if (var.isArray) {
      matVal.val = MaterialUniformValue::Array{var.arraySize};
      auto& arrayVal = std::get<MaterialUniformValue::Array>(matVal.val);
      for (size_t i = 0; i < var.arraySize; ++i) {
        arrayVal[i].val = createDefaultValue(var.typeInfo);
      }
    } else
      matVal.val = createDefaultValue(var.typeInfo);
  };

  MaterialUniformValue::Struct matVals;
  for (const asset::ShaderBlockMember& var : members) {
    MaterialUniformValue& matVal = matVals[var.name];
    recurse(var, matVal);
  }
  return matVals;
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