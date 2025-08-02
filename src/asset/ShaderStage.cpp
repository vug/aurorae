#include "ShaderStage.h"

namespace aur::asset {
ShaderStage ShaderStage::create(ShaderStageDefinition&& def) {
  ShaderStage stage;
  stage.stage_ = def.stage;
  stage.spirVBlob_ = std::move(def.spirv);
  return stage;
}
} // namespace aur::asset