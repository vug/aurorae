#pragma once
#include <assimp/material.h>

enum aiShadingMode;
enum aiPropertyTypeInfo;
enum aiTextureType;
struct aiMaterial;

namespace aur::asset {

const char* getPropertyTypeInfoName(aiPropertyTypeInfo type);
const char* getTextureSemanticName(aiTextureType type);
const char* getShadingModeName(aiShadingMode mode);
void printMaterialProperties(const aiMaterial* mat);

} // namespace aur::asset