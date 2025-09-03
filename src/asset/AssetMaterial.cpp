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
MaterialDefinition::buildDefaultValues(const std::vector<asset::ShaderBlockMember>& members) {
  auto recurse = [](this auto&& self, const asset::ShaderBlockMember& var,
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

Material Material::create(MaterialDefinition&& matDef, Handle<GraphicsProgram> graphProg) {
  Material material;
  material.graphicsProgram_ = graphProg;
  material.materialDef_ = std::move(matDef);
  return material;
}

} // namespace aur::asset
