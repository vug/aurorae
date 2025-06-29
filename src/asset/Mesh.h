#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include "../Utils.h"
#include "../Vertex.h"

namespace aur {
namespace asset {

// A SubMesh,
// A 1D span, a contiguous range in the index memory block of the Mesh at meshIx in a Model
// which can correspond to disconnected geometries/surfaces in 3D space
// they'll be drawn with the Material at materialIx in a Model.
struct DrawSpan {
  u32 meshIx{};
  u32 materialIx{};
  u32 offset{};
  u32 count{};
};

struct Mesh {
  Mesh() = default;

  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::filesystem::path path;
  std::string debugName;
  // TODO(vug): Decouple transform from mesh. An entity in a Scene will have both.
  glm::mat4 transform{1};
};

class Model {
public:
  std::vector<Mesh> meshes;
  // std::vector<Pipeline> -> std::vector<Material>
  std::vector<DrawSpan> drawSpans;

  [[nodiscard]] static Model loadFromFile(const std::filesystem::path& path);
};

}; // namespace asset
} // namespace aur