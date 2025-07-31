#include "ShaderStage.h"

namespace aur::asset {
ShaderStage ShaderStage::create(const ShaderStageDefinition& def) {
  ShaderStage shaderStage;
  shaderStage.stage_ = def.stage;
  shaderStage.spirVBlob_ = std::move(def.spirv);
  return shaderStage;
}
} // namespace aur::asset