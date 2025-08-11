#include "Mesh.h"

#include <ranges>

namespace aur::asset {

MeshDefinition MeshDefinition::makeTriangle() {
  namespace rv = std::ranges::views;
  MeshDefinition def;
  def.vertices.resize(3);
  std::vector<glm::vec3> positions{{0.0f, 1.0f, 0.0f}, {-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}};
  std::vector<glm::vec4> colors{{1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}};
  for (u32 i = 0; i < positions.size(); ++i) {
    def.vertices[i].position = positions[i];
    def.vertices[i].color = colors[i];
    def.indices.push_back(i);
  }
  def.subMeshes = {
      SubMesh{.material = StableId<asset::Material>{"materials/unlit.mat"}, .offset{0}, .count{3}}};
  return def;
}

MeshDefinition MeshDefinition::makeCube() {
  namespace rv = std::ranges::views;
  MeshDefinition def{.indices = {
                         0, 1, 2, 2, 3, 0, // Front face
                         1, 5, 6, 6, 2, 1, // Right face
                         7, 6, 5, 5, 4, 7, // Back face
                         4, 0, 3, 3, 7, 4, // Left face
                         3, 2, 6, 6, 7, 3, // Top face
                         4, 5, 1, 1, 0, 4  // Bottom face
                     }};
  def.vertices.resize(8);
  struct partialVertex {
    glm::vec3 pos;
    glm::vec4 col;
  };
  std::vector<partialVertex> partialData = {
      // Front face
      {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // 0: Red
      {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},  // 1: Green
      {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},   // 2: Blue
      {{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},  // 3: Yellow
      // Back face (indices 4-7 correspond to original indices)
      {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}}, // 4: Magenta
      {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},  // 5: Cyan
      {{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},   // 6: White
      {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}   // 7: Black
  };
  for (const auto& [ix, pVert] : rv::enumerate(partialData)) {
    def.vertices[ix].position = pVert.pos;
    def.vertices[ix].color = pVert.col;
  }
  return def;
}

Mesh Mesh::create(MeshDefinition&& meshDef, std::vector<Handle<Material>>&& materials) {
  Mesh mesh;
  mesh.vertices_ = std::move(meshDef.vertices);
  mesh.indices_ = std::move(meshDef.indices);
  namespace rv = std::ranges::views;

  mesh.materialSpans_ = rv::zip(materials, meshDef.subMeshes) | rv::transform([](const auto& tuple) {
                          return MaterialSpan{
                              .material = std::get<0>(tuple),
                              .offset = std::get<1>(tuple).offset,
                              .count = std::get<1>(tuple).count,
                          };
                        }) |
                        std::ranges::to<std::vector>();
  // TODO(vug): Decouple transform from mesh. An entity in a Scene will have both. (Can be combined with
  //            transform from transform component?)
  mesh.worldFromObject = meshDef.transform;
  return mesh;
}

} // namespace aur::asset