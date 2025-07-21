#pragma once

#include <filesystem>

#include "Mesh.h"
#include "Shader.h"

#include <glaze/glaze/glaze.hpp>
#include <modern-uuid/uuid.h>

namespace aur {
enum class DefinitionType : u32 {
  ShaderStage = 0,
  Shader = 1,
  Material = 2,
  Mesh = 3,
};

// Wrapper class to overcome the glaze issue https://github.com/stephenberry/glaze/issues/1477
// When `to<JSON, NotConvertibleToStringType>` glaze put extra double quotees when the type is used as a key
// in a map To prevent that we derive a thin class that adds `operator std::string_view()` operator, and use
// that one when de/serialization from/to JSON. Also see something slightly relevant:
// https://github.com/stephenberry/glaze/issues/1483
struct glaze_uuid : muuid::uuid {
  using muuid::uuid::uuid;

  operator std::string_view() const {
    thread_local std::string_view buffer;
    buffer = this->to_string();
    return buffer;
  }

  glaze_uuid(const muuid::uuid& u)
      : muuid::uuid(u) {}
  // shouldn't be needed
  // operator muuid::uuid() const { return *this; }
  // glaze_uuid() = default;
};
} // namespace aur

namespace std {
template <>
struct hash<aur::glaze_uuid> {
  size_t operator()(const aur::glaze_uuid& uuid) const noexcept { return std::hash<muuid::uuid>()(uuid); }
};
} // namespace std

namespace glz {
template <>
struct from<JSON, aur::glaze_uuid> {
  template <auto Opts>
  static void op(aur::glaze_uuid& uuid, auto&&... args) {
    // Initialize a string_view with the appropriately sized buffer
    // Alternatively, use a std::string for any size (but this will allocate)
    std::string_view str = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    parse<JSON>::op<Opts>(str, args...);
    uuid = muuid::uuid::from_chars(str).value();
  }
};

template <>
struct to<JSON, aur::glaze_uuid> {
  template <auto Opts>
  static void op(const aur::glaze_uuid& uuid, auto&&... args) noexcept {
    std::string str = uuid.to_string();
    serialize<JSON>::op<Opts>(str, args...);
  }
};
} // namespace glz

template <>
struct glz::meta<aur::DefinitionType> {
  using enum aur::DefinitionType;
  static constexpr auto value = glz::enumerate(ShaderStage, Shader, Material, Mesh);
};

namespace aur {

using AssetUuid = glaze_uuid;

struct AssetEntry {
  DefinitionType type;
  std::filesystem::path srcPath;
  std::filesystem::path dstPath;
  std::optional<std::string> subAssetName;
  std::optional<std::vector<AssetUuid>> dependencies;
};

struct AssetRegistry {
  std::unordered_map<AssetUuid, AssetEntry> entries;
  std::unordered_map<std::string, AssetUuid> aliases;
};

class AssetProcessor {
public:
  // empties the registry and deletes all processed asset files
  static void clearRegistry();

  struct NameSpaces {
    static constexpr muuid::uuid kShaderStage = muuid::uuid("01982b4e-4295-7490-b404-bed575efa867");
  };
  void processAllAssets();
  // load the cache
  void loadRegistry();
  // save the cache
  void saveRegistry();

  void processOnlyNeedingAssets();

  template <typename TDef>
  std::optional<TDef> getDefinition(const std::string& stableSourceIdentifier);

  enum class ShaderBuildMode {
    Debug,
    Release,
  };
  static std::optional<asset::ShaderStageDefinition> processShaderStage(const std::filesystem::path& srcPath,
                                                                        ShaderBuildMode buildMode);

  std::optional<asset::ShaderDefinition> static loadShader(const std::filesystem::path& vertSpirvPath,
                                                           const std::filesystem::path& fragSpirvPath);

  std::vector<asset::MeshDefinition> static processMeshes(const std::filesystem::path& modelPath);

  static bool validateSpirV(const std::vector<u32>& blob);

private:
  static const std::filesystem::path kProcessedAssetsRoot;
  static const std::filesystem::path kRegistryPath;
  AssetRegistry registry_;

  static void initEmptyRegistryFile();
};

} // namespace aur