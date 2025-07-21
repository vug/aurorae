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

#include <glaze/glaze/glaze.hpp>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_reflect.hpp>

namespace aur {
const std::filesystem::path AssetProcessor::kProcessedAssetsRoot{kAssetsFolder / "../processedAssets"};
const std::filesystem::path AssetProcessor::kRegistryPath{kProcessedAssetsRoot / "registry.json"};

void AssetProcessor::initEmptyRegistryFile() {
  if (!std::filesystem::exists(kRegistryPath)) {
    std::filesystem::create_directories(kRegistryPath.parent_path());
    const std::string serializedReg =
        glz::write_json(AssetRegistry{}).value_or("{\"error\": \"Couldn't serialize the registry object.\"}");
    if (!writeBinaryFile(kRegistryPath, glz::prettify_json(serializedReg)))
      log().fatal("Failed to initialize the registry file: {}.", kRegistryPath.string());
  }
}

void AssetProcessor::clearRegistry() {
  const std::uintmax_t removedCnt = std::filesystem::remove_all(kProcessedAssetsRoot);
  log().info("Number of files and directories removed: {}.", removedCnt);

  initEmptyRegistryFile();
}

void AssetProcessor::loadRegistry() {
  if (!std::filesystem::exists(kRegistryPath)) {
    initEmptyRegistryFile();
  }
  std::vector<std::byte> buffer = readBinaryFileBytes(kRegistryPath);

  if (const glz::error_ctx err = glz::read_json(registry_, buffer)) {
    log().fatal("Failed to registry file: {}. error code: {}, msg: {}..", kRegistryPath.string(),
                std::to_underlying(err.ec), err.custom_error_message);
  }
}

void AssetProcessor::saveRegistry() {
  const std::string serializedReg =
      glz::write_json(registry_).value_or("{\"error\": \"Couldn't serialize the registry object.\"}");
  if (!writeBinaryFile(kRegistryPath, glz::prettify_json(serializedReg)))
    log().fatal("Failed to initialize the registry file: {}.", kRegistryPath.string());
}

void AssetProcessor::processAllAssets() {
  for (const std::filesystem::directory_entry& dirEntry :
       std::filesystem::recursive_directory_iterator(kAssetsFolder)) {
    if (!dirEntry.is_regular_file())
      continue;
    if (dirEntry.path().extension() != ".vert")
      continue;
    const auto& srcPath = dirEntry.path();
    log().info("processing asset ingestion file: {}", srcPath.generic_string());
    const ShaderBuildMode buildMode = ShaderBuildMode::Debug;
    std::optional<asset::ShaderStageDefinition> shaderStageDef = processShaderStage(srcPath, buildMode);
    if (shaderStageDef) {
      const std::string serializedDef = glz::write_beve(shaderStageDef.value()).value_or("error");
      const auto dstPath = kProcessedAssetsRoot / srcPath.filename().concat(".shaderStageDef.beve");
      if (!writeBinaryFile(dstPath, serializedDef)) {
        log().warn("Failed to write asset definition to file: {}", srcPath.string());
        continue;
      }

      const std::filesystem::path srcRelPath = std::filesystem::relative(srcPath, kAssetsFolder);
      const std::string stableSourceIdentifier =
          std::format("{}[build:{}]", srcRelPath.generic_string(),
                      buildMode == ShaderBuildMode::Debug ? "Debug" : "Release");
      const muuid::uuid assetId =
          muuid::uuid::generate_sha1(NameSpaces::kShaderStage, stableSourceIdentifier);
      AssetEntry entry{
          .type = DefinitionType::ShaderStage,
          .srcPath = srcPath,
          .dstPath = dstPath,
          .subAssetName = "Debug", // later can also be Release
          .dependencies = std::nullopt,
      };
      registry_.entries.insert({assetId, entry});
      registry_.aliases.insert({stableSourceIdentifier, assetId});
      log().info(">>> processed it to {}", dstPath.string());
    }
  }

  saveRegistry();
}

template <typename TDef>
std::optional<TDef> AssetProcessor::getDefinition(const std::string& stableSourceIdentifier) {
  const auto it = registry_.aliases.find(stableSourceIdentifier);
  if (it == registry_.aliases.end()) {
    log().warn("Asset '{}' is not in the registry.", stableSourceIdentifier);
    return std::nullopt;
  }
  const muuid::uuid& assetId = it->second;
  const AssetEntry& entry = registry_.entries.at(assetId);

  if constexpr (std::is_same_v<TDef, asset::ShaderStageDefinition>) {
    if (entry.type != DefinitionType::ShaderStage)
      log().fatal("Asset '{}' is not a shader stage definition.", stableSourceIdentifier);
  } else
    static_assert(false, "Unimplemented definition type");

  if (!std::filesystem::exists(entry.dstPath))
    log().fatal("Asset in registry '{}' does not have a processed file at {}!", stableSourceIdentifier,
                entry.dstPath.generic_string());
  const std::vector<std::byte> defBuffer = readBinaryFileBytes(entry.dstPath);

  TDef def;
  if (const glz::error_ctx err = glz::read_beve(def, defBuffer)) {
    log().warn("Failed to read asset definition from file: {}. error code: {}, msg: {}. Try reprocessing.",
               entry.dstPath.generic_string(), std::to_underlying(err.ec), err.custom_error_message);
    return std::nullopt;
  }
  return def;
}

template std::optional<asset::ShaderStageDefinition>
AssetProcessor::getDefinition<asset::ShaderStageDefinition>(const std::string& stableSourceIdentifier);

std::optional<asset::ShaderStageDefinition>
AssetProcessor::processShaderStage(const std::filesystem::path& srcPath, ShaderBuildMode buildMode) {
  const std::vector<std::byte> bytes = readBinaryFileBytes(srcPath);
  if (bytes.empty())
    return std::nullopt;
  const std::string_view source(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  const auto [kind,
              stage] = [ext = srcPath.extension().string()]() -> std::pair<shaderc_shader_kind, ShaderStage> {
    if (ext == ".vert")
      return {shaderc_vertex_shader, ShaderStage::Vertex};
    if (ext == ".frag")
      return {shaderc_fragment_shader, ShaderStage::Fragment};
    return {shaderc_glsl_infer_from_source, ShaderStage::Vertex};
  }();

  const shaderc::Compiler compiler;
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

  const shaderc::SpvCompilationResult result =
      compiler.CompileGlslToSpv(source.data(), source.size(), kind, srcPath.string().c_str(), options);
  if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
    log().warn("Error when compiling shader: {}", result.GetErrorMessage());
    return std::nullopt;
  }

  std::vector<u32> spirv(result.cbegin(), result.cend());
  asset::ShaderStageDefinition def{
      .stage = stage,
      .spirv = std::move(spirv),
  };

  const spirv_cross::Compiler comp(def.spirv);
  auto resources = comp.get_shader_resources();
  log().debug("Vertex Inputs:");
  for (const auto& input : resources.stage_inputs) {
    uint32_t loc = comp.get_decoration(input.id, spv::DecorationLocation);
    log().debug("    Location: {}, Name: {}", loc, input.name);
  }

  return def;
}

std::optional<asset::ShaderDefinition>
AssetProcessor::loadShader(const std::filesystem::path& vertSpirvPath,
                           const std::filesystem::path& fragSpirvPath) {

  if (!std::filesystem::exists(vertSpirvPath) || !std::filesystem::exists(fragSpirvPath))
    return {};

  const asset::ShaderDefinition def{
      .vertStageDef = {.stage = ShaderStage::Vertex, .spirv = readBinaryFileU32(vertSpirvPath)},
      .fragStageDef = {.stage = ShaderStage::Fragment, .spirv = readBinaryFileU32(fragSpirvPath)}};

  if (!validateSpirV(def.vertStageDef.spirv) || !validateSpirV(def.fragStageDef.spirv))
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

} // namespace aur