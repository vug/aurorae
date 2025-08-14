#pragma once

#include "../Utils.h"
#include "../VulkanWrappers.h"
#include "AssetTraits.h"
#include <vector>

namespace aur::asset {

enum class ShaderParameterType {
  Unknown,
  // Scalars
  Float,
  Int,
  UInt,
  Bool,
  // Vectors
  Vec2,
  Vec3,
  Vec4,
  // Integer Vectors
  IVec2,
  IVec3,
  IVec4,
  // Matrix
  Mat3,
  Mat4,
  // Other
  Struct,
};

struct ShaderParameter {
  std::string name;
  ShaderParameterType type{};
  u32 binding{};
  u32 offset{};    // For uniform buffer members
  u64 sizeBytes{}; // Size in bytes
  bool isArray{};
  u32 arraySize{};
};

struct ShaderParameterSchema {
  std::vector<ShaderParameter> uniformBufferParams; // From MaterialParams block
  // std::vector<ShaderParameter> textureParams;       // From texture bindings
  // std::vector<ShaderParameter> storageBufferParams; // From storage buffers

  u32 uniformBufferSize{0}; // Total size of MaterialParams block

  [[nodiscard]] bool hasParameter(const std::string& name) const;
  [[nodiscard]] const ShaderParameter* getParameter(const std::string& name) const;
};

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

  [[nodiscard]] static ShaderParameterSchema getSchema(const SpirV& spirV);
  static bool validateSpirV(const std::vector<u32>& blob);

private:
  ShaderStage() = default;
  std::string debugName_;

  ShaderStageType stage_{};
  SpirV spirVBlob_;
};
} // namespace aur::asset