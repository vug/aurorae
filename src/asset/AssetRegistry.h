#pragma once

#include "../Utils.h"
#include "AssetIds.h"
#include "AssetTraits.h"

namespace std {
template <>
struct hash<aur::glaze_uuid> {
  size_t operator()(const aur::glaze_uuid& uuid) const noexcept { return std::hash<muuid::uuid>()(uuid); }
};

template <aur::AssetDefinitionConcept TDefinition>
struct hash<aur::StableId<TDefinition>> {
  size_t operator()(const aur::StableId<TDefinition>& stableId) const noexcept {
    // Use the hash of the underlying string
    return std::hash<std::string>{}(static_cast<const std::string&>(stableId));
  }
};
} // namespace std

namespace aur {
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
  explicit AssetRegistry(const std::filesystem::path& registryFilePath);
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
  [[nodiscard]] std::optional<AssetUuid> getUuid(const std::string& alias) const;
  [[nodiscard]] std::optional<AssetEntry> getEntry(const AssetUuid& uuid) const;
  // Template method for type-safe asset definition retrieval
  template <AssetDefinitionConcept TDefinition>
  [[nodiscard]] std::optional<TDefinition> getDefinition(const AssetUuid& uuid) const;

  [[nodiscard]] inline const std::filesystem::path& getFilePath() const { return filePath_; }

private:
  std::filesystem::path filePath_{kRegistryPath};
  std::unordered_map<AssetUuid, AssetEntry> entries_;
  std::unordered_map<std::string, AssetUuid> aliases_;

  void initEmptyRegistryFile();
  [[nodiscard]] AssetBuildMode buildTypeToAssetBuildMode(BuildType buildType) const;
};
} // namespace aur