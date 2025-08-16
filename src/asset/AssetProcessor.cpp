#include "AssetProcessor.h"

#include <execution>
#include <filesystem>
#include <ranges>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glaze/glaze/glaze.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <shaderc/shaderc.hpp>

#include "../FileIO.h"
#include "../Logger.h"
#include "../Utils.h"
#include "AssetRegistry.h"
#include "GraphicsProgram.h"
#include "Material.h"
#include "Mesh.h"
#include "ShaderReflection.h"
#include "ShaderStage.h"

namespace aur {

AssetProcessor::AssetProcessor(AssetRegistry& registry)
    : registry_{&registry}
    , shaderCompiler_{std::make_unique<shaderc::Compiler>()} {}

// Weird but an empty implementation is needed so that std::unique_ptr<shaderc::Compiler> shaderCompiler_
// can be destructed automatically. And no, it can't be default implementation :-)
AssetProcessor::~AssetProcessor() {} // NOLINT

AssetType AssetProcessor::extensionToDefinitionType(const std::filesystem::path& ext) {
  if (ext == ".vert" || ext == ".frag")
    return AssetType::ShaderStage;
  else if (ext == ".shader")
    return AssetType::GraphicsProgram;
  else if (ext == ".gltf" || ext == ".fbx" || ext == ".usda")
    return AssetType::Mesh;
  else if (ext == ".mat")
    return AssetType::Material;
  else
    log().fatal("Unknown definition type for extension: {}", ext.string());
}

using DefinitionVariant = std::variant<asset::ShaderStageDefinition, asset::GraphicsProgramDefinition,
                                       asset::MaterialDefinition, asset::MeshDefinition>;
using Definitions = std::unordered_map<AssetBuildMode, DefinitionVariant>;

void AssetProcessor::processAllAssets() {
  namespace rv = std::views;
  namespace r = std::ranges;

  const std::unordered_set<std::string_view> kFileExtensionsToProcess = {".vert", ".frag", ".shader", ".mat",
                                                                         ".gltf"};

  const auto assetsByType = std::filesystem::recursive_directory_iterator(kAssetsFolder) |
                            rv::filter([](const std::filesystem::directory_entry& dirEntry) {
                              return dirEntry.is_regular_file();
                            }) |
                            rv::transform(&std::filesystem::directory_entry::path) |
                            rv::filter([&kFileExtensionsToProcess](const auto& path) {
                              return kFileExtensionsToProcess.contains(path.extension().string());
                            }) |
                            rv::transform([this](const auto& path) {
                              return std::pair{extensionToDefinitionType(path.extension()), path};
                            }) |
                            r::to<std::unordered_multimap<AssetType, std::filesystem::path>>();

  for (const AssetType defType : kAssetOrder) {
    log().info("Processing assets of type: {}...", glz::write_json(defType).value_or("unknown"));
    const auto range = assetsByType.equal_range(defType);
    const std::vector<std::filesystem::path> srcPaths =
        r::subrange(range.first, range.second) | rv::transform([](const auto& pair) { return pair.second; }) |
        r::to<std::vector>();

    std::vector<std::optional<AssetEntry>> entries(srcPaths.size());
    std::transform(std::execution::par, srcPaths.begin(), srcPaths.end(), entries.begin(),
                   [this](const auto& srcPath) { return processAssetMakeEntry(srcPath); });

    for (const std::optional<AssetEntry> entryOpt : entries) {
      if (!entryOpt)
        continue;
      registry_->addAlias(entryOpt->alias, entryOpt->uuid);
      registry_->addEntry(entryOpt->uuid, std::move(*entryOpt));
    }
  }

  registry_->save();
  log().info("Processing completed.");
}
std::optional<AssetEntry> AssetProcessor::processAssetMakeEntry(const std::filesystem::path& srcPath) {
  const std::filesystem::path srcRelPath = std::filesystem::relative(srcPath, kAssetsFolder);
  const AssetType defType = extensionToDefinitionType(srcRelPath.extension());
  log().info("   Processing asset ingestion file: {}...", srcPath.generic_string());

  struct ProcessingResult {
    std::unordered_map<AssetBuildMode, DefinitionVariant> definitions;
    std::vector<AssetUuid> dependencies;
    std::string_view extension;
  };
  ProcessingResult result = [this, defType, &srcPath]() -> ProcessingResult {
    switch (defType) {
    case AssetType::ShaderStage: {
      return ProcessingResult{.definitions =
                                  [this, &srcPath]() {
                                    Definitions result;
                                    for (auto [assetMode, shaderMode] :
                                         {std::pair{AssetBuildMode::Debug, ShaderBuildMode::Debug},
                                          std::pair{AssetBuildMode::Release, ShaderBuildMode::Release}})
                                      if (auto defOpt = processShaderStage(srcPath, shaderMode))
                                        result.emplace(assetMode, std::move(*defOpt));
                                    return result;
                                  }(),
                              .extension = "shaderStageDef"};
    }
    case AssetType::GraphicsProgram: {
      return processGraphicsProgram(srcPath)
          .transform([this](asset::GraphicsProgramDefinition def) -> ProcessingResult {
            def.vert.setRegistry(registry_);
            def.frag.setRegistry(registry_);
            const AssetUuid vertUuid = def.vert;
            const AssetUuid fragUuid = def.frag;

            return {
                .definitions = {{AssetBuildMode::Any, std::move(def)}},
                .dependencies = {vertUuid, fragUuid},
                .extension = "graphicsProgramDef",
            };
          })
          .value_or(ProcessingResult{});
    }
    case AssetType::Material: {
      return processMaterial(srcPath)
          .transform([this](asset::MaterialDefinition def) -> ProcessingResult {
            def.graphicsProgram.setRegistry(registry_);
            const AssetUuid progUuid = def.graphicsProgram;

            return {
                .definitions = {{AssetBuildMode::Any, std::move(def)}},
                .dependencies = {progUuid},
                .extension = "materialDef",
            };
          })
          .value_or(ProcessingResult{});
    };
    case AssetType::Mesh: {
      return processMeshes(srcPath)
          .transform([this](asset::MeshDefinition def) -> ProcessingResult {
            for (asset::SubMesh& subMesh : def.subMeshes)
              subMesh.material.setRegistry(registry_);

            std::vector<AssetUuid> materialUuids;
            for (const asset::SubMesh& subMesh : def.subMeshes) {
              materialUuids.push_back(subMesh.material);
            }

            return {
                .definitions = {{AssetBuildMode::Any, std::move(def)}},
                .dependencies = std::move(materialUuids),
                .extension = "meshDef",
            };
          })
          .value_or(ProcessingResult{});
    }
    }
    std::unreachable();
  }();

  if (result.definitions.empty())
    return std::nullopt;

  std::unordered_map<AssetBuildMode, std::filesystem::path> dstVariantRelPaths;
  for (auto& [mode, definition] : result.definitions) {
    std::expected<std::string, glz::error_ctx> serResult =
        std::visit([](auto& def) { return glz::write_beve(def); }, definition);
    if (!serResult.has_value()) {
      log().warn("Failed to serialize definition: {}", serResult.error().custom_error_message);
      continue;
    }
    const std::string serializedDef = serResult.value();
    const std::string_view modeStr = mode == AssetBuildMode::Debug     ? "debug."
                                     : mode == AssetBuildMode::Release ? "release."
                                                                       : "";
    const auto dstRelPath =
        srcRelPath.filename().concat(std::format(".{}.{}beve", result.extension, modeStr));
    const auto dstPath = registry_->getRootFolder() / dstRelPath;
    if (!writeBinaryFile(dstPath, serializedDef)) {
      log().warn("Failed to write asset definition to file: {}", srcPath.generic_string());
      continue;
    }
    dstVariantRelPaths[mode] = dstRelPath;
    log().info("      Processed and saved to {}", dstRelPath.generic_string());
  }

  // TODO(vug): this should be more general. asset::ShaderStage is wrong for other asset types!
  const StableId<asset::ShaderStage> stableSourceIdentifier = srcRelPath.generic_string();
  const muuid::uuid assetId = makeUuid(stableSourceIdentifier);
  return AssetEntry{
      .type = defType,
      .uuid = assetId,
      .alias = std::move(stableSourceIdentifier),
      .srcRelPath = srcRelPath.generic_string(),
      .dstVariantRelPaths = std::move(dstVariantRelPaths),
      .dependencies = [&result]() -> std::optional<std::vector<AssetUuid>> {
        if (result.dependencies.empty())
          return std::nullopt;
        return std::move(result.dependencies);
      }(),
  };
}

// Try this out again! But my hunch tells me that this is mostly to turn off// optimization in a release
// build, and not to turn on optimization in a debug build:
// #pragma optimize("gt", on)
std::optional<asset::ShaderStageDefinition>
AssetProcessor::processShaderStage(const std::filesystem::path& srcPath, ShaderBuildMode buildMode) const {
  const std::vector<std::byte> bytes = readBinaryFileBytes(srcPath);
  if (bytes.empty())
    return std::nullopt;
  const std::string_view source(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  const auto [kind, stage] =
      [ext = srcPath.extension().string()]() -> std::pair<shaderc_shader_kind, ShaderStageType> {
    if (ext == ".vert")
      return {shaderc_vertex_shader, ShaderStageType::Vertex};
    if (ext == ".frag")
      return {shaderc_fragment_shader, ShaderStageType::Fragment};
    return {shaderc_glsl_infer_from_source, ShaderStageType::Vertex};
  }();

  shaderc::CompileOptions options;
  options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
  switch (buildMode) {
  case ShaderBuildMode::Debug:
    options.SetOptimizationLevel(shaderc_optimization_level_zero);
    options.SetGenerateDebugInfo();
    break;
  case ShaderBuildMode::Release:
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    break;
  }
  // options.SetWarningsAsErrors();
  // options.AddMacroDefinition("MY_DEFINE", "1");
  // options.SetIncluder(myIncluderPtr);
  // options.SetAutoBindUniforms(true);

  const shaderc::SpvCompilationResult result = shaderCompiler_->CompileGlslToSpv(
      source.data(), source.size(), kind, srcPath.string().c_str(), options);
  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    log().warn("Error when compiling shader: {}", result.GetErrorMessage());
    return std::nullopt;
  }

  std::vector<u32> spirv(result.cbegin(), result.cend());
  asset::ShaderStageDefinition def{
      .stage = stage,
      .spirv = std::move(spirv),
  };

  if (!asset::ShaderStage::validateSpirV(def.spirv)) {
    log().warn("Invalid SPIR-V generated from: {}", srcPath.generic_string());
    return std::nullopt;
  }
  asset::ShaderStageSchema schema = asset::reflectShaderStageSchema(def.spirv);

  return def;
}
// #pragma optimize("", on)

std::optional<asset::GraphicsProgramDefinition>
AssetProcessor::processGraphicsProgram(const std::filesystem::path& srcPath) {
  if (!std::filesystem::exists(srcPath))
    return std::nullopt;

  const std::vector<std::byte> bytes = readBinaryFileBytes(srcPath);
  asset::GraphicsProgramDefinition def;
  if (const glz::error_ctx err = glz::read_json(def, bytes)) {
    log().warn("Failed to parse '{}' Definition from file: {}. error code: {}, msg: {}. Try "
               "editing the file to fit to correct schema.",
               asset::GraphicsProgram::label, srcPath.generic_string(), std::to_underlying(err.ec),
               err.custom_error_message);
    return std::nullopt;
  }

  return def;
}

std::optional<asset::MaterialDefinition>
AssetProcessor::processMaterial(const std::filesystem::path& srcPath) {
  if (!std::filesystem::exists(srcPath))
    return std::nullopt;

  const std::vector<std::byte> bytes = readBinaryFileBytes(srcPath);
  asset::MaterialDefinition def;
  if (const glz::error_ctx err = glz::read_json(def, bytes)) {
    log().warn("Failed to parse '{}' Definition from file: {}. error code: {}, msg: {}. Try "
               "editing the file to fit to correct schema.",
               asset::Material::label, srcPath.generic_string(), std::to_underlying(err.ec),
               err.custom_error_message);
    return std::nullopt;
  }

  return def;
}

std::optional<asset::MeshDefinition> AssetProcessor::processMeshes(const std::filesystem::path& modelPath) {
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
        def.transform = glm::make_mat4(reinterpret_cast<f32*>(&transform));

        // each aiMesh corresponds to an aur::DrawSpan and includes geometry for that span
        u32 spanOffset{};
        for (const aiMesh* m : aiMeshes) {
          // assert(m->mNumVertices > 0);
          // assert(m->mNumFaces > 0);
          // copy vertex attributes data in this aiMesh to mesh by appending fat vertices
          for (u32 vertIx = 0; vertIx < m->mNumVertices; ++vertIx) {
            Vertex v{};
            const aiVector3D& pos = m->mVertices[vertIx];
            v.position = {pos.x, pos.y, pos.z};
            if (m->HasNormals()) {
              const aiVector3D& norm = m->mNormals[vertIx];
              v.normal = {norm.x, norm.y, norm.z};
            }
            if (m->HasTangentsAndBitangents()) {
              const aiVector3D& tan = m->mTangents[vertIx];
              const aiVector3D& bitan = m->mBitangents[vertIx];
              v.tangent = {tan.x, tan.y, tan.z};
              v.bitangent = {bitan.x, bitan.y, bitan.z};
            }
            if (m->HasVertexColors(0)) {
              const aiColor4D& col0 = m->mColors[0][vertIx];
              v.color = {col0.r, col0.g, col0.b, col0.a};
            }
            if (m->HasTextureCoords(0)) {
              const aiVector3D& uv = m->mTextureCoords[0][vertIx];
              v.texCoord0 = {uv.x, uv.y};
            }
            if (m->HasTextureCoords(1)) {
              const aiVector3D& uv = m->mTextureCoords[1][vertIx];
              v.texCoord1 = {uv.x, uv.y};
            }
            if (m->HasTextureCoords(2)) {
              const aiVector3D& uv = m->mTextureCoords[2][vertIx];
              v.texCoord2 = {uv.x, uv.y};
            }
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

          const StableId<asset::Material> defaultMat{"materials/debug.mat"};
          // TODO(vug): Parse materials from model files and map them to existing materials in the asset
          // library i.e. choose a shader, parse material parameters (constants, textures) and generate a
          // material definition here
          // const std::string tempMatAssetName = std::format(
          //     "material[{}]{}", m->mMaterialIndex,
          //     scene->mMaterials[m->mMaterialIndex]->GetName().C_Str());

          // Record a DrawSpan for this chunk of geometry in the Mesh
          def.subMeshes.emplace_back(
              asset::SubMesh{.material = defaultMat, .offset = spanOffset, .count = aiMeshIndexCnt});
          spanOffset += aiMeshIndexCnt;
        }
      };

  aiMatrix4x4 identity;
  traverse(scene->mRootNode, &identity);

  if (defs.empty())
    return std::nullopt;

  return defs[0];
}

template <AssetConcept TAsset>
AssetUuid AssetProcessor::makeUuid(const StableId<TAsset>& stableId) {
  return muuid::uuid::generate_sha1(TAsset::uuidNamespace, stableId);
}

#define EXPLICITLY_INSTANTIATE_TEMPLATES(TAsset)                                                             \
  template AssetUuid AssetProcessor::makeUuid<TAsset>(const StableId<TAsset>& stableId);
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::GraphicsProgram)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::ShaderStage)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::Material)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::Mesh)
#undef EXPLICITLY_INSTANTIATE_TEMPLATES

} // namespace aur