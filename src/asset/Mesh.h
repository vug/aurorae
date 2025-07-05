#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include "../Handle.h"
#include "../Utils.h"
#include "../Vertex.h"

namespace aur::asset {

struct Material;

struct MeshDefinition {
  struct SubMesh {
    std::string materialAssetName;
    u32 offset{};
    u32 count{};
  };

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

class Mesh {
public:
  static Mesh create(const MeshDefinition& meshDef);

  ~Mesh() = default;
  Mesh(const Mesh& other) = delete;
  Mesh& operator=(const Mesh& other) = delete;
  Mesh(Mesh&& other) noexcept = default;
  Mesh& operator=(Mesh&& other) noexcept = default;

  std::vector<Vertex> getVertices() const { return def_.vertices; }
  std::vector<u32> getIndicates() const { return def_.indices; }

  std::string debugName;

private:
  Mesh() = default;

  MeshDefinition def_;
  // TODO(vug): Decouple transform from mesh. An entity in a Scene will have both.
  glm::mat4 transform{1};
};

} // namespace aur::asset