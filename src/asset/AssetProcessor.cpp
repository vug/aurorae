#include "AssetProcessor.h"

#include <filesystem>
#include <ranges>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/type_ptr.hpp>

#include "../FileIO.h"
#include "../Logger.h"
#include "../Utils.h"
#include "AssimpUtils.h"

namespace aur {

std::optional<asset::ShaderDefinition> AssetProcessor::processShader(const std::filesystem::path& vertPath,
                                                                     const std::filesystem::path& fragPath) {

  if (!std::filesystem::exists(vertPath) || !std::filesystem::exists(fragPath))
    return {};

  const asset::ShaderDefinition def{
      .vertPath = vertPath,
      .fragPath = fragPath,
      .vertBlob = readBinaryFile(vertPath),
      .fragBlob = readBinaryFile(fragPath),
  };

  if (!validateSpirV(def.vertBlob) || !validateSpirV(def.fragBlob))
    return {};

  return def;
}

constexpr u32 kSpirVMagic = 0x07230203; // SPIR-V magic number
std::string getSpirvVersionString(u32 version) {
  const u32 major = (version >> 16) & 0xFFFF; // Upper 16 bits
  const u32 minor = (version >> 8) & 0xFF;    // Next 8 bits
  return std::to_string(major) + "." + std::to_string(minor);
}
std::string getSpirvGeneratorString(uint32_t generator) {
  const u16 vendorID = static_cast<u16>(generator >> 16);
  const u16 toolVersion = static_cast<u16>(generator & 0xFFFF);

  // Map Vendor ID to human-readable names from https://registry.khronos.org/SPIR-V/api/spir-v.xml
  std::string vendorName;
  // clang-format off
  switch (vendorID) {
    case 0: vendorName = "Khronos"; break;
    case 1: vendorName = "LunarG"; break;
    case 2: vendorName = "Valve"; break;
    case 3: vendorName = "Codeplay"; break;
    case 4: vendorName = "NVIDIA"; break;
    case 5: vendorName = "ARM"; break;
    case 6: vendorName = "Khronos - LLVM Generator"; break;
    case 7: vendorName = "Khronos - Assembler"; break;
    case 8: vendorName = "Khronos - Glslang"; break;
    case 17: vendorName = "Khronos - Linker"; break;
    default: vendorName = "Unknown Vendor"; break;
  }
  // clang-format on
  return vendorName + " (Version " + std::to_string(toolVersion) + ")";
}

bool AssetProcessor::validateSpirV(const std::vector<std::byte>& blob) {
  // A SPIR-V should have at least the first 5 words (magic, version, generator, bound, schema)
  if (blob.size() < sizeof(u32) * 5)
    return false;

  // Interpret the blob as u32 words
  const auto words = reinterpret_cast<const u32*>(blob.data());

  if (words[0] != kSpirVMagic)
    return false;

  const u32 version = words[1];
  const u32 generator = words[2];
  const u32 bound = words[3];
  const u32 schema = words[4];
  if (schema != 0) {
    log().warn("SPIR-V schema has to be 0, but is {}.", schema);
    return false;
  }
  log().debug("SPIR-V version: {}, generator: {}, bound: {}", getSpirvVersionString(version),
              getSpirvGeneratorString(generator), bound);

  // Ensure the size of the blob is valid (must contain complete 32-bit words)
  if (blob.size() % sizeof(u32) != 0)
    return false;

  return true;
}

std::vector<asset::MeshDefinition> AssetProcessor::processMeshes(const std::filesystem::path& modelPath) {
  std::vector<asset::MeshDefinition> defs;
  Assimp::Importer importer;

  // aiProcess_MakeLeftHanded flag for RUF import instead of RUB import
  // aiProcess_FlipWindingOrder for CW face winding order instead of CCW
  // aiProcess_FlipUVs to put (0, 0) to top left
  const aiScene* scene = importer.ReadFile(
      modelPath.string().c_str(), aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals |
                                      aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);
  // Could return an empty vector instead
  if (!scene) {
    log().warn("Failed to load mesh from file: {}", modelPath.string());
    return {};
  }

  // TODO(vug): Do something with the materials
  const u32 materialCnt = scene->mNumMaterials;
  log().info("Scene '{}' has {} materials.", scene->mName.C_Str(), materialCnt);
  // asset::printMaterialProperties(mat);

  // aur: Model is made of Meshes and meshes have DrawSpans (Materials)
  // ai: Scene is a Model. Made of Nodes. Each node is a aur::Mesh.
  // a node can have child nodes, and meshes. aiMeshes are DrawSpans.
  std::function<void(const aiNode*, const aiMatrix4x4*)> traverse =
      [&scene, &defs, &traverse](const aiNode* node, const aiMatrix4x4* parentTransform) {
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
        asset::MeshDefinition& def = defs.emplace_back();
        def.vertices.reserve(vertexCnt);
        def.indices.reserve(indexCnt);
        def.objetFromModel = glm::make_mat4(reinterpret_cast<f32*>(&transform));

        // each aiMesh corresponds to an aur::DrawSpan and includes geometry for that span
        u32 spanOffset{};
        for (const aiMesh* m : aiMeshes) {
          // assert(m->mNumVertices > 0);
          // assert(m->mNumFaces > 0);
          // copy vertex attributes data in this aiMesh to mesh by appending fat vertices
          for (u32 vertIx = 0; vertIx < m->mNumVertices; ++vertIx) {
            const aiVector3D& pos = m->mVertices[vertIx];
            const aiColor4D& col0 = m->mColors[0][vertIx];
            const Vertex v{.position = {pos.x, pos.y, pos.z}, .color = {col0.r, col0.g, col0.b, col0.a}};
            def.vertices.push_back(v);
          }

          // copy index data in this aiMesh to mesh by appending its indices
          const std::span faces{m->mFaces, m->mNumFaces};
          u32 aiMeshIndexCnt{};
          for (const aiFace& face : faces) {
            // assert(face.mNumIndices == 3);
            const std::span indices{face.mIndices, face.mNumIndices};
            for (const u32 index : indices)
              def.indices.push_back(index);
            aiMeshIndexCnt += face.mNumIndices;
          }

          // TODO(vug): bring material data to aur too.
          // Record a DrawSpan for this chunk of geometry in the Mesh
          const std::string tempMatAssetName = std::format(
              "material[{}]{}", m->mMaterialIndex, scene->mMaterials[m->mMaterialIndex]->GetName().C_Str());
          def.materialSpans.emplace_back(asset::SubMesh{
              .materialAssetName = tempMatAssetName, .offset = spanOffset, .count = aiMeshIndexCnt});
          spanOffset += aiMeshIndexCnt;
        }
      };

  aiMatrix4x4 identity;
  traverse(scene->mRootNode, &identity);

  return defs;
}

} // namespace aur