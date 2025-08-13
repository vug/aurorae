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
#include "GraphicsProgram.h"
#include "Material.h"
#include "Mesh.h"
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

enum class ShaderParameterType {
  Unknown,
  // Scalars
  Float,
  Int,
  UInt,
  Bool,
  // Vectors
  Vec2,
  Vec3,
  Vec4,
  // Integer Vectors
  IVec2,
  IVec3,
  IVec4,
  // Matrix
  Mat3,
  Mat4,
  // Other
  Struct,
};

ShaderParameterType spirvTypeToShaderParameterType(const spirv_cross::SPIRType& type) {
  switch (type.basetype) {
  case spirv_cross::SPIRType::Float:
    if (type.columns > 1) { // It's a matrix
      if (type.columns == 3 && type.vecsize == 3)
        return ShaderParameterType::Mat3;
      if (type.columns == 4 && type.vecsize == 4)
        return ShaderParameterType::Mat4;
    } else { // It's a scalar or vector
      switch (type.vecsize) {
      case 1:
        return ShaderParameterType::Float;
      case 2:
        return ShaderParameterType::Vec2;
      case 3:
        return ShaderParameterType::Vec3;
      case 4:
        return ShaderParameterType::Vec4;
      }
    }
    break;

  case spirv_cross::SPIRType::Int:
    // Assuming no int matrices, only vectors
    switch (type.vecsize) {
    case 1:
      return ShaderParameterType::Int;
    case 2:
      return ShaderParameterType::IVec2;
    case 3:
      return ShaderParameterType::IVec3;
    case 4:
      return ShaderParameterType::IVec4;
    }
    break;

  case spirv_cross::SPIRType::Boolean:
    // Bools are often represented as ints in uniform blocks,
    // but SPIRV-Cross can identify them.
    if (type.vecsize == 1)
      return ShaderParameterType::Bool;
    // Add bvec2, etc. if needed
    break;

  case spirv_cross::SPIRType::Struct:
    return ShaderParameterType::Struct;

  default:
    return ShaderParameterType::Unknown;
  }
  return ShaderParameterType::Unknown;
}

struct ShaderParameter {
  std::string name;
  ShaderParameterType type{};
  u32 binding{};
  u32 offset{};    // For uniform buffer members
  u64 sizeBytes{}; // Size in bytes
  bool isArray{};
  u32 arraySize{};
};

struct ShaderParameterSchema {
  std::vector<ShaderParameter> uniformBufferParams; // From MaterialParams block
  // std::vector<ShaderParameter> textureParams;       // From texture bindings
  // std::vector<ShaderParameter> storageBufferParams; // From storage buffers

  u32 uniformBufferSize{0}; // Total size of MaterialParams block

  [[nodiscard]] bool hasParameter(const std::string& name) const;
  [[nodiscard]] const ShaderParameter* getParameter(const std::string& name) const;
};

const char* spirVBaseTypeToString(const spirv_cross::SPIRType& type) {
  switch (type.basetype) {
  case spirv_cross::SPIRType::Unknown:
    return "Unknown";
  case spirv_cross::SPIRType::Void:
    return "Void";
  case spirv_cross::SPIRType::Boolean:
    return "Boolean";
  case spirv_cross::SPIRType::SByte:
    return "SByte";
  case spirv_cross::SPIRType::UByte:
    return "UByte";
  case spirv_cross::SPIRType::Short:
    return "Short";
  case spirv_cross::SPIRType::UShort:
    return "UShort";
  case spirv_cross::SPIRType::Int:
    return "Int";
  case spirv_cross::SPIRType::UInt:
    return "UInt";
  case spirv_cross::SPIRType::Int64:
    return "Int64";
  case spirv_cross::SPIRType::UInt64:
    return "UInt64";
  case spirv_cross::SPIRType::Half:
    return "Half";
  case spirv_cross::SPIRType::Float:
    return "Float";
  case spirv_cross::SPIRType::Double:
    return "Double";
  case spirv_cross::SPIRType::Struct:
    return "Struct";
  case spirv_cross::SPIRType::Image:
    return "Image";
  case spirv_cross::SPIRType::SampledImage:
    return "SampledImage";
  case spirv_cross::SPIRType::Sampler:
    return "Sampler";
  case spirv_cross::SPIRType::AccelerationStructure:
    return "AccelerationStructure";
    // Add other types as needed
  default:
    return "Unsupported";
  }
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

  // if (!validateSpirV(def.spirv)) {
  //   log().warn("Invalid SPIR-V generated from: {}", srcPath.generic_string());
  //   return std::nullopt;
  // }

  const spirv_cross::Compiler reflector(def.spirv);
  auto resources = reflector.get_shader_resources();
  log().debug("Vertex Inputs:");
  for (const auto& input : resources.stage_inputs) {
    const spirv_cross::SPIRType& type = reflector.get_type(input.base_type_id);
    const uint32_t location = reflector.get_decoration(input.id, spv::DecorationLocation);
    log().info("Found input: '{} {}' at location {}", spirVBaseTypeToString(type), input.name.c_str(),
               location);

    // spv::DecorationComponent for multiple render targets
    if (reflector.has_decoration(input.id, spv::DecorationFlat)) // NoPerspective, Centroid
      log().info("  - Has 'flat' interpolation qualifier.");

    if (type.basetype == spirv_cross::SPIRType::Struct) {
      // For "in VertexOutput v", this gives "VertexOutput"
      const std::string& structName = reflector.get_name(input.base_type_id);
      log().info("  - It's a struct of type: '{}'", structName.c_str());
      log().info("  - Members:");

      // Iterate over the members of the struct
      for (uint32_t i = 0; i < type.member_types.size(); ++i) {
        const spirv_cross::TypedID memberTypeId = type.member_types[i];
        const spirv_cross::SPIRType memberType = reflector.get_type(memberTypeId);

        const std::string& memberName = reflector.get_member_name(input.base_type_id, i);
        log().info("    - {}[{},{}] {}", spirVBaseTypeToString(memberType), memberType.vecsize,
                   memberType.columns, memberName.c_str());
      }
    }
  }

  ShaderParameterSchema schema;
  for (const spirv_cross::Resource& uniform : resources.uniform_buffers) {
    const u32 set = reflector.get_decoration(uniform.id, spv::DecorationDescriptorSet);
    const u32 binding = reflector.get_decoration(uniform.id, spv::DecorationBinding);

    if (set != 1 || binding != 0)
      continue;

    const std::string_view blockVariableName = uniform.name;
    const std::string_view structName = reflector.get_name(uniform.base_type_id);

    const spirv_cross::SPIRType& blockType = reflector.get_type(uniform.base_type_id);
    const u64 bufferSize = reflector.get_declared_struct_size(blockType);

    for (uint32_t i = 0; i < blockType.member_types.size(); ++i) {
      const auto& memberTypeId = blockType.member_types[i];
      const auto& memberType = reflector.get_type(memberTypeId);

      ShaderParameter param;

      param.name = reflector.get_member_name(uniform.type_id, i); // "vizMode"
      auto foo = reflector.get_member_name(blockType.self, i);

      param.type = spirvTypeToShaderParameterType(reflector.get_type(blockType.member_types[i]));
      param.offset = reflector.type_struct_member_offset(blockType, i);
      size_t member_offset = reflector.get_member_decoration(blockType.self, i, spv::DecorationOffset);
      param.sizeBytes = reflector.get_declared_struct_member_size(blockType, i);
      param.binding = binding;

      // param.type = spirvTypeToParameterType(memberType);
      param.isArray = !memberType.array.empty();
      if (param.isArray)
        param.arraySize = memberType.array[0];

      schema.uniformBufferParams.push_back(param);
    }

    // Log all the names we found
    log().info("Found uniform block:");
    log().info("  Variable name: '{}'", blockVariableName); // "matParams"
    log().info("  Struct name: '{}'", structName);          // "MaterialParams"
    log().info("  Members:");
    for (const auto& param : schema.uniformBufferParams) {
      log().info("    - {}", param.name); // "vizMode"
    }
  }

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