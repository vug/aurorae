#include "../Logger.h"
#include "GraphicsProgram.h"
#include "Material.h"

namespace aur::asset {

Material Material::create(MaterialDefinition&& matDef, Handle<GraphicsProgram> graphProg) {
  Material material;
  material.graphicsProgram_ = graphProg;
  material.materialDef_ = std::move(matDef);
  material.uniformInfos_ =
      buildUniformInfos(graphProg->getCombinedSchema().getMaterialUniformBufferSchema()->members);
  return material;
}

std::unordered_map<std::string, UniformInfo>
Material::buildUniformInfos(const std::vector<ShaderBlockMember>& members) {

  std::unordered_map<std::string, UniformInfo> infos;
  auto recurse = [&infos](this auto&& self, const ShaderBlockMember& var,
                          const std::string& parentName) -> void {
    const bool isStruct = var.typeInfo.baseType == ShaderVariableTypeInfo::BaseType::Struct;
    if (isStruct && var.isArray) {
      for (u32 itemIx = 0; itemIx < var.arraySize; ++itemIx) {
        const std::string name = std::format("{}[{}]", parentName, itemIx);
        UniformInfo& info = infos[name];
        info.offset = var.offset + itemIx * var.arrayStride;
        info.sizeBytes = var.sizeBytes;
        for (ShaderBlockMember child : var.members)
          self(child, name);
      }
    } else if (isStruct) {
      const std::string name = std::format("{}.{}", parentName, var.name);
      UniformInfo& info = infos[name];
      info.offset = var.offset;
      info.sizeBytes = var.sizeBytes;
      for (ShaderBlockMember child : var.members) {
        self(child, name);
      }
    } else if (var.isArray) {
      for (u32 itemIx = 0; itemIx < var.arraySize; ++itemIx) {
        const std::string name = std::format("{}[{}]", parentName, itemIx);
        UniformInfo& info = infos[name];
        info.offset = var.offset + itemIx * var.arrayStride;
        info.sizeBytes = var.sizeBytes;
      }
    } else {
      const std::string name = std::format("{}.{}", parentName, var.name);
      UniformInfo& info = infos[name];
      info.offset = var.offset;
      info.sizeBytes = var.sizeBytes;
    }
  };

  for (const ShaderBlockMember& var : members)
    recurse(var, "");

  std::map<std::string, UniformInfo> sortedInfos;
  for (const auto& [name, info] : infos) {
    sortedInfos[name] = info;
  }
  for (const auto& [name, info] : sortedInfos) {
    log().info("{}: {}, {}", name, info.offset, info.sizeBytes);
  }

  return infos;
}

} // namespace aur::asset
