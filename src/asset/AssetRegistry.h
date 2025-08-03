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
  AssetUuid uuid;
  // Relative to asset folder
  std::filesystem::path srcRelPath;
  // Relative to registry root folder
  std::unordered_map<AssetBuildMode, std::filesystem::path> dstVariantRelPaths;
  // std::chrono::system_clock::time_point lastProcessed;
  std::optional<std::vector<AssetUuid>> dependencies;
};

class AssetRegistry {
public:
  static const std::filesystem::path kDefaultProcessedAssetsRoot;
  static const std::filesystem::path kDefaultRegistryPath;
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

  [[nodiscard]] inline const std::filesystem::path& getRootFolder() const { return registryRootFolder_; }
  [[nodiscard]] inline const std::filesystem::path& getPath() const { return registryPath_; }

private:
  std::filesystem::path registryRootFolder_{kDefaultProcessedAssetsRoot};
  std::filesystem::path registryPath_{kDefaultRegistryPath};
  std::unordered_map<AssetUuid, AssetEntry> entries_;
  std::unordered_map<std::string, AssetUuid> aliases_;

  void initEmptyRegistryFile();
  [[nodiscard]] AssetBuildMode buildTypeToAssetBuildMode(BuildType buildType) const;
};

// AssetRef can hold either a UUID or a string and converts between them automatically
class AssetRef {
public:
  enum class Mode {
    Invalid,
    AssetUuid,
    StableId,
  };

  // Constructors
  AssetRef() = default;
  explicit AssetRef(const AssetUuid& uuid)
      : uuid_(uuid)
      , mode_(Mode::AssetUuid) {}
  explicit AssetRef(const std::string& stableId)
      : stableId_(stableId)
      , mode_(Mode::StableId) {}
  explicit AssetRef(std::string_view stableId)
      : stableId_(stableId)
      , mode_(Mode::StableId) {}

  operator AssetUuid() const;
  operator std::string() const;

  // Getters
  const AssetUuid& getUuid() const { return static_cast<AssetUuid>(*this); }
  const std::string& getStableId() const { return static_cast<std::string>(*this); }

  // Context injection for registry access
  void setRegistry(const AssetRegistry* registry) { registry_ = registry; }

  bool hasUuid() const { return mode_ == Mode::AssetUuid; }
  bool hasStableId() const { return mode_ == Mode::StableId; }

private:
  AssetUuid uuid_;
  std::string stableId_;
  Mode mode_{Mode::Invalid};

  // Context for conversion - injected during serialization
  mutable const AssetRegistry* registry_ = nullptr;

  // friend class AssetRefSerializer; // For Glaze integration
};
} // namespace aur