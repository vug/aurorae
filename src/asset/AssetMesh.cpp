#include "Mesh.h"
#include <glm/gtc/type_ptr.hpp>

namespace aur::asset {

MeshDefinition MeshDefinition::makeTriangle() {
  return {.vertices =
              {
                  {{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},   // Bottom vertex (Red)
                  {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}}, // Right top vertex (Green)
                  {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}}   // Left top vertex (Blue)
              },
          .indices = {0, 1, 2},
          .subMeshes = {
              SubMesh{.material = StableId<asset::Material>{"materials/unlit.mat"}, .offset{0}, .count{3}}}};
}
MeshDefinition MeshDefinition::makeCube() {
  return {.vertices =
              {
                  // Front face
                  {{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // 0: Red
                  {{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},  // 1: Green
                  {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},   // 2: Blue
                  {{-1.0f, 1.0f, 1.0f},
                   {1.0f, 1.0f, 0.0f, 1.0f}}, // 3: Yellow
                                              // Back face (indices 4-7 correspond to original indices)
                  {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}}, // 4: Magenta
                  {{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},  // 5: Cyan
                  {{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},   // 6: White
                  {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}   // 7: Black
              },
          .indices = {
              0, 1, 2, 2, 3, 0, // Front face
              1, 5, 6, 6, 2, 1, // Right face
              7, 6, 5, 5, 4, 7, // Back face
              4, 0, 3, 3, 7, 4, // Left face
              3, 2, 6, 6, 7, 3, // Top face
              4, 5, 1, 1, 0, 4  // Bottom face
          }};
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