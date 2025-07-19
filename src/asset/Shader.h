#pragma once

#include <filesystem>
#include <functional>

#include "../Handle.h"
#include "../VulkanWrappers.h"

namespace aur::asset {

struct ShaderStageDefinition {
  ShaderStage stage;
  std::filesystem::path sourcePath;
  std::vector<u32> spirv;
};

struct ShaderDefinition {
  // std::string vertStageDef;
  // std::string fragStageDef;
  std::filesystem::path vertPath;
  std::filesystem::path fragPath;
  std::vector<std::byte> vertBlob;
  std::vector<std::byte> fragBlob;
  std::vector<u32> spirv;
};

class Shader {
public:
  static Shader create(const ShaderDefinition& shaderDef);

  ~Shader() = default;

  Shader(const Shader& other) = delete;
  Shader& operator=(const Shader& other) = delete;
  Shader(Shader&& other) noexcept = default;
  Shader& operator=(Shader&& other) noexcept = default;

  [[nodiscard]] const ShaderDefinition& getDefinition() const { return def_; }
  [[nodiscard]] const std::vector<std::byte>& getVertexBlob() const { return def_.vertBlob; }
  [[nodiscard]] const std::vector<std::byte>& getFragmentBlob() const { return def_.fragBlob; }
  [[nodiscard]] const std::string& getDebugName() const { return debugName; }

private:
  Shader() = default;

  ShaderDefinition def_;
  std::string debugName;
};

using ShaderUpdateCallback = std::function<void(Handle<asset::Shader>)>;

} // namespace aur::asset
