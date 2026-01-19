#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glaze/glaze.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include "../Handle.h"
#include "../Utils.h"
#include "../Vertex.h"
#include "AssetTraits.h"

namespace aur::asset {

class Material;

struct SubMesh {
  AssetRef material;
  u32 offset{};
  u32 count{};
};

struct MeshDefinition {
  // Increment version after each change to the schema or processing logic
  static constexpr u32 schemaVersion{1};
  u32 version{schemaVersion};

  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::vector<SubMesh> subMeshes;
  glm::mat4 transform{1};

  static MeshDefinition makeTriangle();
  // TODO(vug): quads are used a lot, could be nice to generate them procedurally
  // static MeshDefinition makeQuad();
  static MeshDefinition makeCube();
};

// A SubMesh,
// A 1D span, a contiguous range in the index memory block of the Mesh at meshIx in a Model
// which can correspond to disconnected geometries/surfaces in 3D space
// they'll be drawn with the Material at materialIx in a Model.
struct MaterialSpan {
  Handle<Material> material{};
  u32 offset{};
  u32 count{};
};

class Mesh : public AssetTypeMixin<Mesh, MeshDefinition, AssetType::Mesh, "Mesh",
                                   "019870da-b469-7073-8c5d-c09cdb24c657"> {
public:
  static Mesh create(MeshDefinition&& meshDef, std::vector<Handle<Material>>&& materials);

  ~Mesh() = default;
  Mesh(const Mesh& other) = delete;
  Mesh& operator=(const Mesh& other) = delete;
  Mesh(Mesh&& other) noexcept = default;
  Mesh& operator=(Mesh&& other) noexcept = default;

  [[nodiscard]] std::vector<Vertex> getVertices() const { return vertices_; }
  [[nodiscard]] std::vector<u32> getIndicates() const { return indices_; }
  [[nodiscard]] std::vector<MaterialSpan> getMaterialSpans() const { return materialSpans_; }

  std::string debugName;

private:
  Mesh() = default;
  std::string debugName_;

  std::vector<Vertex> vertices_;
  std::vector<u32> indices_;
  std::vector<MaterialSpan> materialSpans_;
  // TODO(vug): Decouple transform from mesh. An entity in a Scene will have both. (Can be combined with
  //            transform from transform component?)
  glm::mat4 worldFromObject{1};
};

} // namespace aur::asset

namespace glz {

template <>
struct from<BEVE, glm::mat3> {
  template <auto Opts>
  static void op(glm::mat3& mat, auto&&... args) {
    std::array<float, 9> matElements;
    parse<BEVE>::op<Opts>(matElements, args...);
    std::memcpy(glm::value_ptr(mat), matElements.data(), sizeof(float) * 9);
  }
};

template <>
struct to<BEVE, glm::mat3> {
  template <auto Opts>
  static void op(const glm::mat3& mat, auto&&... args) noexcept {
    std::span matSpan(glm::value_ptr(mat), 9);
    serialize<BEVE>::op<Opts>(matSpan, args...);
  }
};

template <>
struct from<BEVE, glm::mat4> {
  template <auto Opts>
  static void op(glm::mat4& mat, auto&&... args) {
    std::array<float, 16> matElements;
    parse<BEVE>::op<Opts>(matElements, args...);
    std::memcpy(glm::value_ptr(mat), matElements.data(), sizeof(float) * 16);
  }
};

template <>
struct to<BEVE, glm::mat4> {
  template <auto Opts>
  static void op(const glm::mat4& mat, auto&&... args) noexcept {
    std::span matSpan(glm::value_ptr(mat), 16);
    serialize<BEVE>::op<Opts>(matSpan, args...);
  }
};

template <>
struct from<BEVE, glm::vec2> {
  template <auto Opts>
  static void op(glm::vec2& vec, auto&&... args) {
    std::span<float, 2> components(glm::value_ptr(vec), 2);
    parse<BEVE>::op<Opts>(components, args...);
  }
};

template <>
struct to<BEVE, glm::vec2> {
  template <auto Opts>
  static void op(const glm::vec2& vec, auto&&... args) noexcept {
    std::span components(glm::value_ptr(vec), 2);
    serialize<BEVE>::op<Opts>(components, args...);
  }
};

template <>
struct from<BEVE, glm::vec3> {
  template <auto Opts>
  static void op(glm::vec3& vec, auto&&... args) {
    std::span<float, 3> components(glm::value_ptr(vec), 3);
    parse<BEVE>::op<Opts>(components, args...);
  }
};

template <>
struct to<BEVE, glm::vec3> {
  template <auto Opts>
  static void op(const glm::vec3& vec, auto&&... args) noexcept {
    std::span components(glm::value_ptr(vec), 3);
    serialize<BEVE>::op<Opts>(components, args...);
  }
};

template <>
struct from<BEVE, glm::vec4> {
  template <auto Opts>
  static void op(glm::vec4& vec, auto&&... args) {
    std::span<float, 4> components(glm::value_ptr(vec), 4);
    parse<BEVE>::op<Opts>(components, args...);
  }
};

template <>
struct to<BEVE, glm::vec4> {
  template <auto Opts>
  static void op(const glm::vec4& vec, auto&&... args) noexcept {
    std::span components(glm::value_ptr(vec), 4);
    serialize<BEVE>::op<Opts>(components, args...);
  }
};
} // namespace glz