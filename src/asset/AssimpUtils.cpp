#include "AssimpUtils.h"

#include <assimp/material.h>

#include "../Logger.h"
#include "../Utils.h"

namespace aur::asset {
const char* getPropertyTypeInfoName(aiPropertyTypeInfo type) {
  switch (type) {
  case aiPTI_Float:
    return "Float";
  case aiPTI_Double:
    return "Double";
  case aiPTI_String:
    return "String";
  case aiPTI_Integer:
    return "Integer";
  case aiPTI_Buffer:
    return "Buffer";
  default:
    return "UNKNOWN_PROPERTY_TYPE_INFO";
  }
}

const char* getTextureSemanticName(aiTextureType type) {
  switch (type) {
  case aiTextureType_NONE:
    return "NONE";
  case aiTextureType_DIFFUSE:
    return "DIFFUSE";
  case aiTextureType_SPECULAR:
    return "SPECULAR";
  case aiTextureType_AMBIENT:
    return "AMBIENT";
  case aiTextureType_EMISSIVE:
    return "EMISSIVE";
  case aiTextureType_HEIGHT:
    return "HEIGHT";
  case aiTextureType_NORMALS:
    return "NORMALS";
  case aiTextureType_SHININESS:
    return "SHININESS";
  case aiTextureType_OPACITY:
    return "OPACITY";
  case aiTextureType_DISPLACEMENT:
    return "DISPLACEMENT";
  case aiTextureType_LIGHTMAP:
    return "LIGHTMAP";
  case aiTextureType_REFLECTION:
    return "REFLECTION";
  case aiTextureType_BASE_COLOR: // PBR materials
    return "BASE_COLOR";
  case aiTextureType_NORMAL_CAMERA: // PBR materials
    return "NORMAL_CAMERA";
  case aiTextureType_EMISSION_COLOR: // PBR materials
    return "EMISSION_COLOR";
  case aiTextureType_METALNESS: // PBR materials
    return "METALNESS";
  case aiTextureType_AMBIENT_OCCLUSION: // PBR materials
    return "AMBIENT_OCCLUSION";
  case aiTextureType_SHEEN:
    return "SHEEN";
  case aiTextureType_CLEARCOAT:
    return "CLEARCOAT";
  case aiTextureType_TRANSMISSION:
    return "TRANSMISSION";
  case aiTextureType_UNKNOWN:
    return "UNKNOWN";
  default:
    return "UNHANDLED_TEXTURE_TYPE";
  }
}

const char* getShadingModeName(aiShadingMode mode) {
  switch (mode) {
  case aiShadingMode_Flat:
    return "Flat";
  case aiShadingMode_Gouraud:
    return "Gouraud";
  case aiShadingMode_Phong:
    return "Phong";
  case aiShadingMode_Blinn:
    return "Blinn";
  case aiShadingMode_Toon:
    return "Toon";
  case aiShadingMode_OrenNayar:
    return "OrenNayar";
  case aiShadingMode_Minnaert:
    return "Minnaert";
  case aiShadingMode_CookTorrance:
    return "CookTorrance";
  case aiShadingMode_NoShading:
    return "NoShading";
  case aiShadingMode_Fresnel:
    return "Fresnel";
  // PBR shading modes
  case aiShadingMode_PBR_BRDF:
    return "PBR_BRDF";
  default:
    return "UNKNOWN_SHADING_MODE";
  }
}

void printMaterialProperties(const aiMaterial* mat) {
  log().info("Material {} has {} properties.", mat->GetName().C_Str(), mat->mNumProperties);

  for (unsigned int i = 0; i < mat->mNumProperties; ++i) {
    const aiMaterialProperty* prop = mat->mProperties[i];

    log().info("  Property {}:", i);
    log().info("    Key: {}", prop->mKey.C_Str());
    log().info("    Texture Type Semantic: {}",
               asset::getTextureSemanticName(static_cast<aiTextureType>(prop->mSemantic)));
    log().info("    Index: {}", prop->mIndex);
    log().info("    Type: {}", asset::getPropertyTypeInfoName(prop->mType));

    // Method 1) for each property, use aiMaterial::Get() templated with correct value types over all material
    // keys https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html#constants
    if (aiString name; mat->Get(AI_MATKEY_NAME, name) == aiReturn_SUCCESS)
      log().info("   (material name) value: {}.", name.C_Str());
    else if (aiColor3D baseColor; mat->Get(AI_MATKEY_BASE_COLOR, baseColor) == aiReturn_SUCCESS)
      log().info("   (color 3/4) value: ({}, {}, {})", baseColor.r, baseColor.g, baseColor.b);
    else if (aiColor3D colorDiffuse; mat->Get(AI_MATKEY_COLOR_DIFFUSE, colorDiffuse) == aiReturn_SUCCESS)
      log().info("   (color 3/4) value: ({}, {}, {})", colorDiffuse.r, colorDiffuse.g, colorDiffuse.b);

    // Method 2) use aiGetMaterialFloat, aiGetMaterialString etc based on mType.
    // See Array functions.

    // Method 3) get it from mData directly. Needs to utilize mDataLength
    // const char* keyName = prop->mKey.C_Str();
    // aiPropertyTypeInfo dataType = prop->mType;
    // if mType is aiPTI_Float:
    // float value = *(float*)prop->mData;

    // const std::string keyStr = prop->mKey.C_Str();
    // // Determine the type and print the value
    // switch (prop->mType) {
    // case aiPTI_Float: {
    //   f32* floatVal{};
    //   u32* pMax{};
    //   if (mat->Get(prop->mKey.C_Str(), prop->mType, prop->mIndex, floatVal, pMax) != aiReturn_SUCCESS)
    //     log().fatal("Failed to get float property {}.", prop->mKey.C_Str());
    //   // Check for specific multi-float properties first
    //   if (keyStr == AI_MATKEY_COLOR_DIFFUSE || keyStr == AI_MATKEY_COLOR_SPECULAR ||
    //       keyStr == AI_MATKEY_COLOR_AMBIENT || keyStr == AI_MATKEY_COLOR_EMISSIVE ||
    //       keyStr == AI_MATKEY_COLOR_TRANSPARENT || keyStr == AI_MATKEY_COLOR_REFLECTIVE) {
    //     if (prop->mDataLength == sizeof(aiColor4D)) {
    //       const aiColor4D colorVal = *reinterpret_cast<const aiColor4D*>(prop->mData);
    //       log().info("    Value: RGBA({}, {}, {}, {}), (raw size): {} bytes", colorVal.r, colorVal.g,
    //                  colorVal.b, colorVal.a, prop->mDataLength);
    //     } else if (prop->mDataLength == sizeof(aiColor3D)) {
    //       // Some older models might store colors as aiColor3D
    //       aiColor3D colorVal;
    //       std::memcpy(&colorVal, prop->mData, sizeof(aiColor3D));
    //       log().info("    Value: RGB({}, {}, {}), (raw size): {} bytes", colorVal.r, colorVal.g,
    //       colorVal.b,
    //                  prop->mDataLength);
    //     } else {
    //       log().info("    Value: (Invalid color data length), (raw size): {} bytes", prop->mDataLength);
    //     }
    //   } else if (keyStr == AI_MATKEY_SHININESS || keyStr == AI_MATKEY_OPACITY ||
    //              keyStr == AI_MATKEY_REFRACTI || keyStr == AI_MATKEY_BUMPSCALING // PBR
    //   ) {
    //     // Handle single float properties
    //     if (prop->mDataLength == sizeof(float)) {
    //       const float floatVal = *reinterpret_cast<const float*>(prop->mData);
    //       log().info("    Value: {}, (raw size): {} bytes", floatVal, prop->mDataLength);
    //     } else {
    //       log().info("    Value: (Invalid float data length), (raw size): {} bytes", prop->mDataLength);
    //     }
    //   } else {
    //     // Fallback for other float properties not specifically handled
    //     log().info("    Value: (Unhandled float key/length. Raw data shown), (raw size): {} bytes",
    //                prop->mDataLength);
    //     // Optionally, print raw bytes or some partial floats if you want
    //     // For example, if it's a vector:
    //     if (prop->mDataLength % sizeof(float) == 0) {
    //       const size_t num_floats = prop->mDataLength / sizeof(float);
    //       std::string float_vals_str = "[";
    //       for (size_t f_idx = 0; f_idx < num_floats; ++f_idx) {
    //         float f_val;
    //         std::memcpy(&f_val, static_cast<const char*>(prop->mData) + f_idx * sizeof(float),
    //         sizeof(float)); float_vals_str += std::to_string(f_val); if (f_idx < num_floats - 1)
    //           float_vals_str += ", ";
    //       }
    //       float_vals_str += "]";
    //       log().info("    Parsed Floats: {}", float_vals_str);
    //     }
    //   }
    // } break;
    // case aiPTI_Double: {
    //   if (prop->mDataLength == sizeof(double)) {
    //     const double doubleVal = *reinterpret_cast<const double*>(prop->mData);
    //     log().info("    Value: {}, (raw size): {} bytes", doubleVal, prop->mDataLength);
    //   } else {
    //     log().info("    Value: (Invalid double data length), (raw size): {} bytes", prop->mDataLength);
    //   }
    // } break;
    // case aiPTI_String: {
    //   if (prop->mDataLength >= sizeof(uint32_t)) { // At least enough for the length prefix
    //     const aiString* ai_str = reinterpret_cast<const aiString*>(prop->mData);
    //     log().info("    Value: \"{}\", (raw size): {} bytes", ai_str->C_Str(), prop->mDataLength);
    //   } else {
    //     log().info("    Value: (Invalid string data), (raw size): {} bytes", prop->mDataLength);
    //   }
    // } break;
    // case aiPTI_Integer: {
    //   if (prop->mDataLength == sizeof(int)) {
    //     const int intVal = *reinterpret_cast<const int*>(prop->mData);
    //     log().info("    Value: {}, (raw size): {} bytes", intVal, prop->mDataLength);
    //   } else {
    //     log().info("    Value: (Invalid integer data length), (raw size): {} bytes", prop->mDataLength);
    //   }
    // } break;
    // case aiPTI_Buffer: {
    //   // Special handling for known properties like shading model
    //   if (keyStr == AI_MATKEY_SHADING_MODEL && prop->mDataLength == sizeof(uint32_t)) {
    //     uint32_t shadingModeVal;
    //     std::memcpy(&shadingModeVal, prop->mData, sizeof(uint32_t));
    //     aiShadingMode mode = static_cast<aiShadingMode>(shadingModeVal);
    //     log().info("    Value: {} ({}), (raw size): {} bytes", shadingModeVal,
    //                asset::getShadingModeName(mode), prop->mDataLength);
    //   } else if (keyStr == AI_MATKEY_TWOSIDED && prop->mDataLength == sizeof(int)) {
    //     int twoSided;
    //     std::memcpy(&twoSided, prop->mData, sizeof(int));
    //     log().info("    Value: {} (two-sided), (raw size): {} bytes", twoSided, prop->mDataLength);
    //   }
    //   // Add more specific handling for other known buffer-type material keys here if needed
    //   else {
    //     // Fallback: Print generic buffer content as hex bytes
    //     const u32 max_print_bytes = 32;
    //     u32 bytes_to_print = std::min(prop->mDataLength, max_print_bytes);
    //
    //     std::string buffer_str = "0x";
    //     for (size_t byte_idx = 0; byte_idx < bytes_to_print; ++byte_idx) {
    //       char byte_val[3];
    //       sprintf(byte_val, "%02X", static_cast<unsigned char>(prop->mData[byte_idx]));
    //       buffer_str += byte_val;
    //     }
    //     if (prop->mDataLength > max_print_bytes) {
    //       buffer_str += "...";
    //     }
    //     log().info("    Value: {} (raw buffer), (raw size): {} bytes", buffer_str, prop->mDataLength);
    //   }
    // } break;
    // default:
    //   log().fatal("unsupported assimp material property type.");
    // }
  }
}

} // namespace aur::asset