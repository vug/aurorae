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
  // TODO(vug): quads are used a lot, could be nice to generate them procedurally
  // static Mesh makeQuad();
  static Mesh makeCube();
};

}; // namespace asset
} // namespace aur