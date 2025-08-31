#pragma once

#include "../Handle.h"
#include "../VulkanWrappers.h"
#include "AssetTraits.h"

namespace aur {
struct MaterialUniformValue {
  using Array = std::vector<MaterialUniformValue>;
  using Struct = std::map<std::string, MaterialUniformValue, std::less<>>;
  std::variant<i32, u32, f32, glm::vec2, glm::vec3, glm::vec4, glm::mat3, glm::mat4, Struct, Array> val{};
};

} // namespace aur
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

template <>
struct from<JSON, aur::MaterialUniformValue> {
  template <auto Opts>
  static void op(aur::MaterialUniformValue& value, auto&&... args) {
    parse<JSON>::op<Opts>(value.val, args...);
  }
};

template <>
struct to<JSON, aur::MaterialUniformValue> {
  template <auto Opts>
  static void op(const aur::MaterialUniformValue& value, auto&&... args) noexcept {
    serialize<JSON>::op<Opts>(value.val, args...);
  }
};

template <>
struct from<BEVE, aur::MaterialUniformValue> {
  template <auto Opts>
  static void op(aur::MaterialUniformValue& value, auto&&... args) {
    parse<BEVE>::op<Opts>(value.val, args...);
  }
};

template <>
struct to<BEVE, aur::MaterialUniformValue> {
  template <auto Opts>
  static void op(const aur::MaterialUniformValue& value, auto&&... args) noexcept {
    serialize<BEVE>::op<Opts>(value.val, args...);
  }
};

template <>
struct meta<std::variant<aur::i32, aur::u32, aur::f32, glm::vec2, glm::vec3, glm::vec4, glm::mat3, glm::mat4,
                         aur::MaterialUniformValue::Struct, aur::MaterialUniformValue::Array>> {
  static constexpr std::string_view tag = "valueType";
  static constexpr auto ids =
      std::array{"int", "uint", "float", "vec2", "vec3", "vec4", "mat3", "mat4", "struct", "array"};
};

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
  MaterialUniformValue::Struct values;
  // MaterialMetadata using which we can create the PipelineCreateInfo
  // Schema of material parameters, their types (options, ranges, texture, numbers, vec2s etc) and stored
  // values. Then, Renderer::getOrCreateMaterial() takes this create info and creates a
  // renderer::GraphicsProgram, a Pipeline object, and corresponding buffers and images
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

private:
  Material() = default;
  std::string debugName_;

  Handle<GraphicsProgram> graphicsProgram_;
  MaterialDefinition materialDef_;
};

} // namespace aur::asset
