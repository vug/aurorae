#pragma once

#include "../Handle.h"
#include "../VulkanWrappers.h"
#include "AssetTraits.h"
#include "ShaderReflection.h"
#include <glm/glm.hpp>

namespace glz {
template <>
struct meta<glm::vec2> {
  using T = glm::vec2;
  static constexpr auto value = object(&T::x, &T::y);
};
template <>
struct meta<glm::vec3> {
  using T = glm::vec3;
  static constexpr auto value = object(&T::x, &T::y, &T::z);
};
template <>
struct meta<glm::vec4> {
  using T = glm::vec4;
  static constexpr auto value = object(&T::x, &T::y, &T::z, &T::w);
};

template <uint32_t Format, auto Opts>
static void mat3serialize(const glm::mat3& mat, auto&&... args) noexcept {
  std::map<std::string, glm::vec3> values{
      {"col0", mat[0]},
      {"col1", mat[1]},
      {"col2", mat[2]},
  };
  serialize<Format>::template op<Opts>(values, args...);
}
template <uint32_t Format, auto Opts>
static void mat3parse(glm::mat3& mat, auto&&... args) {
  std::map<std::string, glm::vec3> values;
  parse<Format>::template op<Opts>(values, args...);

  mat = glm::mat3(values["col0"], values["col1"], values["col2"]);
}
template <>
struct to<JSON, glm::mat3> {
  template <auto Opts>
  static void op(const glm::mat3& mat, auto&&... args) noexcept {
    mat3serialize<JSON, Opts>(mat, args...);
  };
};
template <>
struct from<JSON, glm::mat3> {
  template <auto Opts>
  static void op(glm::mat3& mat, auto&&... args) {
    mat3parse<JSON, Opts>(mat, args...);
  }
};
// template <>
// struct meta<glm::mat3> {
//   using T = glm::mat3;
//   static constexpr auto value = array(1, 2, 3, 4, 5, 6, 7, 8, 9);
// };

template <uint32_t Format, auto Opts>
static void mat4serialize(const glm::mat4& mat, auto&&... args) noexcept {
  std::map<std::string, glm::vec4> values{
      {"col0", mat[0]},
      {"col1", mat[1]},
      {"col2", mat[2]},
      {"col3", mat[3]},
  };
  serialize<Format>::template op<Opts>(values, args...);
}
template <uint32_t Format, auto Opts>
static void mat4parse(glm::mat4& mat, auto&&... args) {
  std::map<std::string, glm::vec4> values;
  parse<Format>::template op<Opts>(values, args...);

  mat = glm::mat4(values["col0"], values["col1"], values["col2"], values["col3"]);
}
template <>
struct to<JSON, glm::mat4> {
  template <auto Opts>
  static void op(const glm::mat4& mat, auto&&... args) noexcept {
    mat4serialize<JSON, Opts>(mat, args...);
  };
};
template <>
struct from<JSON, glm::mat4> {
  template <auto Opts>
  static void op(glm::mat4& mat, auto&&... args) {
    mat4parse<JSON, Opts>(mat, args...);
  }
};
// template <>
// struct meta<glm::mat4> {
//   static constexpr auto value = glz::array(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
// };

} // namespace glz

namespace aur {
enum class BlendingPreset {
  NoBlend,    // Opaque
  AlphaBlend, // Transparent
  Additive,
  // AlphaToCoverage,
  // PremultipliedAlpha,
  // Overlay,
  // Subtractive,
  // Xor, // Highlight
  // Invert,
  // And, // Mask
};
} // namespace aur
template <>
struct glz::meta<aur::BlendingPreset> {
  using enum aur::BlendingPreset;
  static constexpr auto value = glz::enumerate(NoBlend, AlphaBlend, Additive);
};

namespace aur::asset {
class GraphicsProgram;
struct Pipeline;

struct MaterialParameters {
  std::unordered_map<std::string, i32> integers;
  std::unordered_map<std::string, f32> floats;
  std::unordered_map<std::string, glm::vec2> vector2s;
  std::unordered_map<std::string, glm::vec3> vector3s;
  std::unordered_map<std::string, glm::vec4> vector4s;
  std::unordered_map<std::string, glm::mat3> matrix3s;
  std::unordered_map<std::string, glm::mat4> matrix4s;
};

struct MaterialDefinition {
  // Increment version after each change to the schema or processing logic
  static constexpr u32 schemaVersion{1};
  u32 version{schemaVersion};

  AssetRef graphicsProgram;
  bool depthTest{true};
  bool depthWrite{true};
  PolygonMode polygonMode{PolygonMode::Fill};
  CullMode cullMode{CullMode::Back};
  FrontFace frontFace{FrontFace::CounterClockwise};
  f32 lineWidth{1.0f};
  BlendingPreset blendPreset{BlendingPreset::NoBlend};
  MaterialParameters matParams;
};

struct UniformInfo {
  u64 offset{};
  u64 sizeBytes{};
};

class Material : public AssetTypeMixin<Material, MaterialDefinition, AssetType::Material, "Material",
                                       "019870da-2c87-7f9e-aece-9484ce47cac9"> {
public:
  static Material create(MaterialDefinition&& matDef, Handle<GraphicsProgram> graphProg);

  ~Material() = default;
  Material(const Material&) = delete;
  Material& operator=(const Material&) = delete;
  Material(Material&& other) noexcept = default;
  Material& operator=(Material&& other) noexcept = default;

  [[nodiscard]] inline const Handle<GraphicsProgram>& getGraphicsProgramHandle() const {
    return graphicsProgram_;
  }
  [[nodiscard]] inline const MaterialDefinition& getDefinition() const { return materialDef_; }

  static constexpr u32 kUniformParamsBinding{0};
  static constexpr u32 kMaterialParamsSet{1};

  static std::unordered_map<std::string, UniformInfo>
  buildUniformInfos(const std::vector<ShaderBlockMember>& members);

private:
  Material() = default;
  std::string debugName_;

  Handle<GraphicsProgram> graphicsProgram_;
  MaterialDefinition materialDef_;
  std::unordered_map<std::string, UniformInfo> uniformInfos_;
};

} // namespace aur::asset
