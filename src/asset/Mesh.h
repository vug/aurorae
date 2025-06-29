#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include "../Utils.h"
#include "../Vertex.h"
#include "Handle.h"

namespace aur {
namespace asset {

struct Material;

// A SubMesh,
// A 1D span, a contiguous range in the index memory block of the Mesh at meshIx in a Model
// which can correspond to disconnected geometries/surfaces in 3D space
// they'll be drawn with the Material at materialIx in a Model.
struct MaterialSpan {
  Handle<Material> material{};
  u32 offset{};
  u32 count{};
};

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::vector<MaterialSpan> materialSpans;
  std::string debugName;
  // TODO(vug): Decouple transform from mesh. An entity in a Scene will have both.
  glm::mat4 transform{1};

  static Mesh makeTriangle();
  // static Mesh makeQuad();
  static Mesh makeCube();
};

class Model {
public:
  std::vector<Mesh> meshes;
  // std::vector<Pipeline> -> std::vector<Material>
  std::vector<MaterialSpan> drawSpans;

  [[nodiscard]] static Model loadFromFile(const std::filesystem::path& path);
};

}; // namespace asset
} // namespace aur