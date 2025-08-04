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
#include <spirv_cross/spirv_reflect.hpp>

#include "../FileIO.h"
#include "../Logger.h"
#include "../Utils.h"
#include "AssetRegistry.h"

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
  else
    log().fatal("Unknown definition type for extension: {}", ext.string());
}

using DefinitionVariant =
    std::variant<asset::ShaderStageDefinition, asset::GraphicsProgramDefinition>; // , asset::MeshDefinition
using Definitions = std::unordered_map<AssetBuildMode, DefinitionVariant>;

void AssetProcessor::processAllAssets() {
  namespace rv = std::views;
  namespace r = std::ranges;

  const std::unordered_set<std::string_view> kFileExtensionsToProcess = {".vert", ".frag", ".shader"};

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
      log().fatal("Not implemented yet.");
    } break;
    case AssetType::Mesh: {
      log().fatal("Not implemented yet.");
    } break;
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
  options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
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

  // if (!validateSpirV(def.spirv)) {
  //   log().warn("Invalid SPIR-V generated from: {}", srcPath.generic_string());
  //   return std::nullopt;
  // }

  // const spirv_cross::Compiler comp(def.spirv);
  // auto resources = comp.get_shader_resources();
  // log().debug("Vertex Inputs:");
  // for (const auto& input : resources.stage_inputs) {
  //   uint32_t loc = comp.get_decoration(input.id, spv::DecorationLocation);
  //   log().debug("    Location: {}, Name: {}", loc, input.name);
  // }

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
    log().warn("Failed to generate GraphicsProgramDefinition from file: {}. error code: {}, msg: {}. Try "
               "editing the "
               "file to fit to correct schema.",
               srcPath.generic_string(), std::to_underlying(err.ec), err.custom_error_message);
    return std::nullopt;
  }

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

bool AssetProcessor::validateSpirV(const std::vector<u32>& blob) {
  // A SPIR-V should have at least the first 5 words (magic, version, generator, bound, schema)
  if (blob.size() < 5)
    return false;

  const u32* words = blob.data();

  if (words[0] != kSpirVMagic)
    return false;

  const u32 version = words[1];
  const u32 generator = words[2];
  const u32 bound = words[3];
  if (const u32 schema = words[4]; schema != 0) {
    log().warn("SPIR-V schema has to be 0, but is {}.", schema);
    return false;
  }
  log().debug("SPIR-V version: {}, generator: {}, bound: {}", getSpirvVersionString(version),
              getSpirvGeneratorString(generator), bound);

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

template <AssetConcept TAsset>
AssetUuid AssetProcessor::makeUuid(const StableId<TAsset>& stableId) {
  return muuid::uuid::generate_sha1(AssetRegistry::NameSpaces::kShaderStage, stableId);
}

// A structure to hold the relevant information about a single variable
// in the interface between shader stages (e.g., a `vec3` at `location = 0`).
struct ShaderInterfaceVariable {
  uint32_t location; // layout(location = N)
  spirv_cross::SPIRType::BaseType type;
  uint32_t vecsize;
  uint32_t columns;
  std::string name;

  // We primarily sort by location to ensure a consistent order for comparison.
  bool operator<(const ShaderInterfaceVariable& other) const { return location < other.location; }

  // For validation, we require location, type, and dimensions to be identical.
  // The name is not used for comparison as it can be optimized away or differ,
  // but it's useful for logging and debugging.
  bool operator==(const ShaderInterfaceVariable& other) const {
    return location == other.location && type == other.type && vecsize == other.vecsize &&
           columns == other.columns;
  }
};

/**
 * @brief Converts a SPIRType::BaseType enum to its string representation.
 *
 * @param type The base type enum from a SPIRV-Cross type query.
 * @return A string describing the type (e.g., "Float", "Int").
 */
std::string basetype_to_string(spirv_cross::SPIRType::BaseType type) {
  using BaseType = spirv_cross::SPIRType::BaseType;
  switch (type) {
  case BaseType::Unknown:
    return "Unknown";
  case BaseType::Void:
    return "Void";
  case BaseType::Boolean:
    return "Boolean";
  case BaseType::Char:
    return "Char";
  case BaseType::Int:
    return "Int";
  case BaseType::UInt:
    return "UInt";
  case BaseType::Int64:
    return "Int64";
  case BaseType::UInt64:
    return "UInt64";
  case BaseType::Half:
    return "Half";
  case BaseType::Float:
    return "Float";
  case BaseType::Double:
    return "Double";
  case BaseType::Struct:
    return "Struct";
  case BaseType::Image:
    return "Image";
  case BaseType::SampledImage:
    return "SampledImage";
  case BaseType::Sampler:
    return "Sampler";
  case BaseType::AccelerationStructure:
    return "AccelerationStructure";
  default:
    return "Other";
  }
}

// A helper function to print the details of an interface variable.
static void printVariable(const ShaderInterfaceVariable& var) {
  log().info("  - Location: {}, Type: {}, , VecSize: {}, Columns: {}, Name: {}", var.location,
             basetype_to_string(var.type), var.vecsize, var.columns, var.name);
}

/**
 * @brief Extracts a list of interface variables from a shader's resources.
 *
 * @param compiler The SPIRV-Cross compiler instance for the shader.
 * @param resources The list of resources to inspect (e.g., stage_inputs or stage_outputs).
 * @return A vector of extracted interface variables.
 */
static std::vector<ShaderInterfaceVariable>
extractInterfaceVariables(const spirv_cross::Compiler& compiler,
                          const spirv_cross::SmallVector<spirv_cross::Resource>& resources) {

  std::vector<ShaderInterfaceVariable> variables;

  for (const auto& resource : resources) {
    // We must ignore built-in variables like gl_Position, gl_FragCoord, etc.,
    // as they are part of the system interface, not the user-defined one.
    if (compiler.has_decoration(resource.id, spv::DecorationBuiltIn)) {
      continue;
    }

    const auto& type = compiler.get_type(resource.type_id);

    variables.push_back({.location = compiler.get_decoration(resource.id, spv::DecorationLocation),
                         .type = type.basetype,
                         .vecsize = type.vecsize,
                         .columns = type.columns,
                         .name = resource.name});
  }

  // Sort the variables by location. This provides a canonical order, making
  // the comparison between vertex outputs and fragment inputs straightforward.
  std::sort(variables.begin(), variables.end());

  return variables;
}

// --- Main Demonstration ---
// In your actual code, you would pass your compiled SPIR-V blobs here.
bool validate_shader_linkage(const std::vector<uint32_t>& vertex_spirv,
                             const std::vector<uint32_t>& fragment_spirv) {

  // 1. Create compiler instances for both shader stages.
  spirv_cross::Compiler vert_compiler(vertex_spirv);
  spirv_cross::Compiler frag_compiler(fragment_spirv);

  // 2. Extract the shader resources (lists of inputs, outputs, uniforms, etc.).
  spirv_cross::ShaderResources vert_resources = vert_compiler.get_shader_resources();
  spirv_cross::ShaderResources frag_resources = frag_compiler.get_shader_resources();

  // 3. Extract the interface variables we care about:
  //    - For the Vertex Shader, we get its STAGE OUTPUTS.
  //    - For the Fragment Shader, we get its STAGE INPUTS.
  std::vector<ShaderInterfaceVariable> vert_outputs =
      extractInterfaceVariables(vert_compiler, vert_resources.stage_outputs);

  std::vector<ShaderInterfaceVariable> frag_inputs =
      extractInterfaceVariables(frag_compiler, frag_resources.stage_inputs);

  // 4. Print the results for inspection.
  log().info("Vertex Shader Outputs");
  for (const auto& var : vert_outputs)
    printVariable(var);

  log().info("Fragment Shader Inputs");
  for (const auto& var : frag_inputs)
    printVariable(var);

  // 5. The validation step:
  // Because we sorted the variables by location, we can simply compare the vectors.
  // If they are identical, the interface contract is met.
  if (vert_outputs == frag_inputs) {
    log().info("[SUCCESS] Vertex and Fragment shader interfaces match.");
    return true;
  } else {
    log().info("[FAILURE] Mismatch between Vertex outputs and Fragment inputs!");
    return false;
  }
}

#define EXPLICITLY_INSTANTIATE_TEMPLATES(TAsset)                                                             \
  template AssetUuid AssetProcessor::makeUuid<TAsset>(const StableId<TAsset>& stableId);
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::GraphicsProgram)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::ShaderStage)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::Material)
EXPLICITLY_INSTANTIATE_TEMPLATES(asset::Mesh)
#undef EXPLICITLY_INSTANTIATE_TEMPLATES

} // namespace aur