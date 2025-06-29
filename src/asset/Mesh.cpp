#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <ranges>
#include <span>

#include "../Logger.h"

#include <glm/gtc/type_ptr.hpp>

namespace aur {
namespace asset {

Model Model::loadFromFile(const std::filesystem::path& path) {
  Assimp::Importer importer;

  // aiProcess_MakeLeftHanded flag for RUF import instead of RUB import
  // aiProcess_FlipWindingOrder for CW face winding order instead of CCW
  // aiProcess_FlipUVs to put (0, 0) to top left
  const aiScene* scene = importer.ReadFile(
      path.string().c_str(), aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals |
                                 aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
  if (!scene)
    log().fatal("Failed to load mesh from file: {}", path.string());

  Model model;
  // aur: Model is made of Meshes and meshes have DrawSpans (Materials)
  // ai: Scene is a Model. Made of Nodes. Each node is a aur::Mesh.
  // a node can have child nodes, and meshes. aiMeshes are DrawSpans.
  std::function<void(const aiNode*, const aiMatrix4x4*)> traverse =
      [&scene, &model, &traverse](const aiNode* node, const aiMatrix4x4* parentTransform) {
        // post-order traversal for no reason
        aiMatrix4x4 transform = *parentTransform * node->mTransformation;
        for (u32 i = 0; i < node->mNumChildren; ++i)
          traverse(node->mChildren[i], &transform);

        // if there are no aiMeshes in this aiNode, we don't have to read geometry from it
        if (node->mNumMeshes == 0)
          return;

        // count vertices and indices in this aiNode / aur::Mesh for allocation
        namespace rv = std::views;
        namespace r = std::ranges;
        auto aiMeshes = std::span(node->mMeshes, node->mNumMeshes) |
                        rv::transform([&scene](u32 ix) { return scene->mMeshes[ix]; });
        const u32 vertexCnt = r::fold_left(aiMeshes | rv::transform(&aiMesh::mNumVertices), 0u, std::plus{});
        const u32 indexCnt = r::fold_left(aiMeshes | rv::transform([](const aiMesh* m) {
                                            return std::span<const aiFace>{m->mFaces, m->mNumFaces};
                                          }) | rv::join |
                                              rv::transform(&aiFace::mNumIndices),
                                          0u, std::plus{});

        // each aiNode correspond to an aur::Mesh
        Mesh& mesh = model.meshes.emplace_back();
        mesh.vertices.reserve(vertexCnt);
        mesh.indices.reserve(indexCnt);
        mesh.transform = glm::make_mat4(reinterpret_cast<f32*>(&transform));

        // each aiMesh corresponds to an aur::DrawSpan and includes geometry for that span
        model.drawSpans.reserve(node->mNumMeshes);
        u32 spanOffset{};
        for (const aiMesh* m : aiMeshes) {
          // assert(m->mNumVertices > 0);
          // assert(m->mNumFaces > 0);
          // copy vertex attributes data in this aiMesh to mesh by appending fat vertices
          for (u32 vertIx = 0; vertIx < m->mNumVertices; ++vertIx) {
            const aiVector3D& pos = m->mVertices[vertIx];
            const aiColor4D& col0 = m->mColors[0][vertIx];
            Vertex v{{pos.x, pos.y, pos.z}, {col0.r, col0.g, col0.b, col0.a}};
            mesh.vertices.push_back(v);
          }

          // copy index data in this aiMesh to mesh by appending its indices
          std::span faces{m->mFaces, m->mNumFaces};
          u32 aiMeshIndexCnt{};
          for (const aiFace& face : faces) {
            // assert(face.mNumIndices == 3);
            std::span indices{face.mIndices, face.mNumIndices};
            for (u32 index : indices)
              mesh.indices.push_back(index);
            aiMeshIndexCnt += face.mNumIndices;
          }

          // TODO(vug): bring material data to aur too.
          // Record a DrawSpan for this chunk of geometry in the Mesh
          model.drawSpans.emplace_back(
              MaterialSpan{.material = Handle<Material>{0}, .offset = spanOffset, .count = aiMeshIndexCnt});
          spanOffset += aiMeshIndexCnt;
        }
      };

  const aiMatrix4x4 identity{};
  traverse(scene->mRootNode, &identity);

  return model;
}

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