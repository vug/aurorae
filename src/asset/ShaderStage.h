#pragma once

#include "../Utils.h"
#include "../VulkanWrappers.h"
#include <vector>

namespace aur::asset {

using SpirV = std::vector<u32>;

struct ShaderStageDefinition {
  ShaderStageType stage;
  SpirV spirv;
};

class ShaderStage {
public:
  static ShaderStage create(ShaderStageDefinition&& shaderDef);

  ~ShaderStage() = default;

  ShaderStage(const ShaderStage& other) = delete;
  ShaderStage& operator=(const ShaderStage& other) = delete;
  ShaderStage(ShaderStage&& other) noexcept = default;
  ShaderStage& operator=(ShaderStage&& other) noexcept = default;

  [[nodiscard]] const ShaderStageType& getStage() const { return stage_; }
  [[nodiscard]] const SpirV& getSpirVBlob() const { return spirVBlob_; }
  [[nodiscard]] const std::string& getDebugName() const { return debugName_; }

private:
  ShaderStage() = default;

  ShaderStageType stage_;
  SpirV spirVBlob_;
  std::string debugName_;
};
} // namespace aur::asset