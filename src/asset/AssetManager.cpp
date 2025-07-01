#include "AssetManager.h"

#include <filesystem>
#include <ranges>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/type_ptr.hpp>

#include "../Logger.h"
#include "AssimpUtils.h"
#include "Mesh.h"

namespace aur {

Handle<asset::Shader> AssetManager::loadShaderFromFile(const std::filesystem::path& path) {
  // could return an invalid handle instead
  if (!std::filesystem::exists(path))
    log().fatal("Shader file not found: {}", path.string());

  asset::Shader& shader = shaders_.emplace_back(path);
  shader.filePath = path;

  return Handle<asset::Shader>{static_cast<u32>(shaders_.size() - 1)};
}

std::vector<Handle<asset::Mesh>> AssetManager::loadMeshFromFile(const std::filesystem::path& path) {
  Assimp::Importer importer;

  // aiProcess_MakeLeftHanded flag for RUF import instead of RUB import
  // aiProcess_FlipWindingOrder for CW face winding order instead of CCW
  // aiProcess_FlipUVs to put (0, 0) to top left
  const aiScene* scene = importer.ReadFile(
      path.string().c_str(), aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals |
                                 aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
  // Could return an empty vector instead
  if (!scene)
    log().fatal("Failed to load mesh from file: {}", path.string());

  // TODO(vug): Do something with the materials
  const u32 materialCnt = scene->mNumMaterials;
  log().info("Scene {} has {} materials.", scene->mName.C_Str(), materialCnt);
  auto materials = scene->mMaterials;
  const aiMaterial* mat = materials[0];
  asset::printMaterialProperties(mat);

  std::vector<Handle<asset::Mesh>> meshes;
  // aur: Model is made of Meshes and meshes have DrawSpans (Materials)
  // ai: Scene is a Model. Made of Nodes. Each node is a aur::Mesh.
  // a node can have child nodes, and meshes. aiMeshes are DrawSpans.
  std::function<void(const aiNode*, const aiMatrix4x4*)> traverse =
      [this, &scene, &meshes, &traverse](const aiNode* node, const aiMatrix4x4* parentTransform) {
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
        asset::Mesh& mesh = meshes_.emplace_back();
        mesh.vertices.reserve(vertexCnt);
        mesh.indices.reserve(indexCnt);
        mesh.transform = glm::make_mat4(reinterpret_cast<f32*>(&transform));

        // each aiMesh corresponds to an aur::DrawSpan and includes geometry for that span
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
          mesh.materialSpans.emplace_back(asset::MaterialSpan{
              .material = Handle<asset::Material>{0}, .offset = spanOffset, .count = aiMeshIndexCnt});
          spanOffset += aiMeshIndexCnt;
        }

        meshes.push_back(Handle<asset::Mesh>{static_cast<u32>(meshes_.size() - 1)});
      };

  aiMatrix4x4 identity;
  traverse(scene->mRootNode, &identity);

  return meshes;
}
} // namespace aur