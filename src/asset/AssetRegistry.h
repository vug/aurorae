#pragma once

#include <glaze/glaze/glaze.hpp>
#include <modern-uuid/uuid.h>

#include "../Utils.h"
#include "AssetTraits.h"
#include "Common.h"

namespace aur {

enum class DefinitionType : u32 {
  ShaderStage = 0,
  Shader = 1,
  Material = 2,
  Mesh = 3,
};

// Wrapper class to overcome the glaze issue https://github.com/stephenberry/glaze/issues/1477
// When `to<JSON, NotConvertibleToStringType>` glaze put extra double quotes when the type is used as a key
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

enum class AssetBuildMode {
  // To be used by assets for which the build mode does not matter, such as Mesh, Texture.
  Any = 0,
  // To be used by assets for which the build mode matters, such as ShaderStage.
  Debug = 1,
  Release = 2,
};

struct AssetEntry {
  DefinitionType type;
  std::filesystem::path srcPath;
  std::unordered_map<AssetBuildMode, std::filesystem::path> dstVariantPaths;
  // std::chrono::system_clock::time_point lastProcessed;
  std::optional<std::vector<AssetUuid>> dependencies;
};

class AssetRegistry {
public:
  static const std::filesystem::path kProcessedAssetsRoot;
  static const std::filesystem::path kRegistryPath;
  struct NameSpaces {
    static constexpr muuid::uuid kShaderStage = muuid::uuid("01982b4e-4295-7490-b404-bed575efa867");
  };

  AssetRegistry() = default;
  AssetRegistry(const std::filesystem::path& registryFilePath);
  // empties the registry and deletes all processed asset files
  void clear();
  void load();
  void save() const;

  // Asset entry management
  void addEntry(const AssetUuid& uuid, const AssetEntry& entry);
  void addAlias(const std::string& alias, const AssetUuid& uuid);
  [[nodiscard]] bool hasEntry(const AssetUuid& uuid) const;
  [[nodiscard]] bool hasAlias(const std::string& alias) const;
  [[nodiscard]] size_t getEntryCount() const;

  // Lookup methods
  [[nodiscard]] std::optional<AssetUuid> findAssetByAlias(const std::string& alias) const;
  [[nodiscard]] std::optional<AssetEntry> getEntry(const AssetUuid& uuid) const;

  // Template method for type-safe asset definition retrieval
  template <AssetDefinition TDefinition>
  [[nodiscard]] std::optional<TDefinition>
  getDefinition(const StableId<TDefinition>& stableSourceIdentifier) const;

  [[nodiscard]] inline const std::filesystem::path& getFilePath() const { return filePath_; }

private:
  std::filesystem::path filePath_{kRegistryPath};
  std::unordered_map<AssetUuid, AssetEntry> entries_;
  std::unordered_map<std::string, AssetUuid> aliases_;

  void initEmptyRegistryFile();
  AssetBuildMode buildTypeToAssetBuildMode(BuildType buildType) const;
};
} // namespace aur