#pragma once

#include "../Utils.h"
#include "../VulkanWrappers.h"
#include "AssetTraits.h"
#include <vector>

namespace aur::asset {

using SpirV = std::vector<u32>;

struct ShaderStageDefinition {
  // Increment version after each change to the schema or processing logic
  static constexpr u32 schemaVersion{1};
  u32 version{schemaVersion};

  ShaderStageType stage;
  SpirV spirv;
};

class ShaderStage : public AssetTypeMixin<ShaderStage, ShaderStageDefinition, AssetType::ShaderStage,
                                          "ShaderStage", "019870da-db28-7b80-a02c-f01dadc1ada2"> {
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

  static bool validateSpirV(const std::vector<u32>& blob);

private:
  ShaderStage() = default;
  std::string debugName_;

  ShaderStageType stage_{};
  SpirV spirVBlob_;
};
} // namespace aur::asset