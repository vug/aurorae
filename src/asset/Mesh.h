#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include "../Handle.h"
#include "../Utils.h"
#include "../Vertex.h"
#include "AssetTraits.h"

namespace aur::asset {

class Material;

struct SubMesh {
  std::string materialAssetName;
  u32 offset{};
  u32 count{};
};

struct MeshDefinition {
  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::vector<SubMesh> materialSpans;
  glm::mat4 objetFromModel{1};

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
  static Mesh create(MeshDefinition&& meshDef);

  ~Mesh() = default;
  Mesh(const Mesh& other) = delete;
  Mesh& operator=(const Mesh& other) = delete;
  Mesh(Mesh&& other) noexcept = default;
  Mesh& operator=(Mesh&& other) noexcept = default;

  [[nodiscard]] std::vector<Vertex> getVertices() const { return def_.vertices; }
  [[nodiscard]] std::vector<u32> getIndicates() const { return def_.indices; }
  [[nodiscard]] std::vector<SubMesh> getSubMeshes() const { return def_.materialSpans; }

  std::string debugName;

private:
  Mesh() = default;
  std::string debugName_;

  MeshDefinition def_;
  // TODO(vug): Decouple transform from mesh. An entity in a Scene will have both.
  glm::mat4 transform{1};
};

} // namespace aur::asset