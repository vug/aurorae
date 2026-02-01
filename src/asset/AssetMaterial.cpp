#include "../Logger.h"
#include "GraphicsProgram.h"
#include "Material.h"

namespace aur::asset {

MaterialUniformValue::Variant
MaterialDefinition::createDefaultValue(const asset::ShaderVariableTypeInfo& typeInfo) {
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
}

MaterialUniformValue::Struct
MaterialDefinition::buildDefaultValues(const std::vector<ShaderBlockMember>& members) {
  auto recurse = [](this auto&& self, const ShaderBlockMember& var, MaterialUniformValue& matVal) -> void {
    if (var.typeInfo.baseType == ShaderVariableTypeInfo::BaseType::Struct) {
      matVal.val = MaterialUniformValue::Struct{};
      auto& structData = std::get<MaterialUniformValue::Struct>(matVal.val);
      for (const ShaderBlockMember& child : var.members) {
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
  for (const ShaderBlockMember& var : members) {
    MaterialUniformValue& matVal = matVals[var.name];
    recurse(var, matVal);
  }
  return matVals;
}

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
