#include "Mesh.h"
#include <glm/gtc/type_ptr.hpp>

namespace aur {
namespace asset {

Mesh Mesh::makeTriangle() {
  return Mesh{.vertices =
                  {
                      {{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},   // Bottom vertex (Red)
                      {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}}, // Right top vertex (Green)
                      {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}}   // Left top vertex (Blue)
                  },
              .indices = {0, 1, 2},
              .debugName = "Procedural Triangle"};
}
Mesh Mesh::makeCube() {
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
          .indices =
              {
                  0, 1, 2, 2, 3, 0, // Front face
                  1, 5, 6, 6, 2, 1, // Right face
                  7, 6, 5, 5, 4, 7, // Back face
                  4, 0, 3, 3, 7, 4, // Left face
                  3, 2, 6, 6, 7, 3, // Top face
                  4, 5, 1, 1, 0, 4  // Bottom face
              },
          .debugName = "Procedural Cube"};
}

} // namespace asset
} // namespace aur