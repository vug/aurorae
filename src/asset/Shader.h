#pragma once

#include <filesystem>

namespace aur::asset {

struct ShaderDefinition {
  std::filesystem::path vertPath;
  std::filesystem::path fragPath;
};

class Shader {
public:
  static std::optional<Shader> create(const ShaderDefinition& shaderDef);

  ~Shader() = default;

  Shader(const Shader& other) = delete;
  Shader& operator=(const Shader& other) = delete;
  Shader(Shader&& other) noexcept;
  Shader& operator=(Shader&& other) noexcept;

  [[nodiscard]] const std::vector<std::byte>& getVertBlob() const { return vertBlob; }
  [[nodiscard]] const std::vector<std::byte>& getFragBlob() const { return fragBlob; }
  [[nodiscard]] const std::string& getDebugName() const { return debugName; }

private:
  Shader() = default;
  std::vector<std::byte> vertBlob;
  std::vector<std::byte> fragBlob;

  std::string debugName;
};

} // namespace aur::asset
