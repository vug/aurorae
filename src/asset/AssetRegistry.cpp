#include "AssetRegistry.h"

#include "../FileIO.h"
#include "../Logger.h"
#include "GraphicsProgram.h"

#include <glaze/glaze/glaze.hpp>

namespace glz {

// Additions to glz::meta makes enums human-readable
template <>
struct meta<aur::AssetBuildMode> {
  using enum aur::AssetBuildMode;
  static constexpr auto value = glz::enumerate(Any, Debug, Release);
};

} // namespace glz

namespace aur {

// ---------------------------------------------------------------

const std::filesystem::path AssetRegistry::kDefaultProcessedAssetsRoot{kAssetsFolder / "../processedAssets"};
const std::filesystem::path AssetRegistry::kDefaultRegistryPath{kDefaultProcessedAssetsRoot /
                                                                "registry.json"};

// Create a temporary struct to match the original serialization format
struct RegistryData {
  std::unordered_map<AssetUuid, AssetEntry> entries;
  std::unordered_map<std::string, AssetUuid> aliases;
};

void AssetRegistry::initEmptyRegistryFile() {
  if (!std::filesystem::exists(registryPath_)) {
    std::filesystem::create_directories(registryPath_.parent_path());

    const std::string serializedReg =
        glz::write_json(RegistryData{}).value_or("{\"error\": \"Couldn't serialize the registry object.\"}");
    if (!writeBinaryFile(registryPath_, glz::prettify_json(serializedReg)))
      log().fatal("Failed to initialize the registry file: {}.", registryPath_.string());
  }
}

AssetRegistry::AssetRegistry(const std::filesystem::path& registryFilePath)
    : registryRootFolder_(registryFilePath.parent_path())
    , registryPath_(registryFilePath) {}

void AssetRegistry::clear() {
  entries_.clear();
  aliases_.clear();

  const std::uintmax_t removedCnt = std::filesystem::remove_all(kDefaultProcessedAssetsRoot);
  log().info("Number of files and directories removed: {}.", removedCnt);

  initEmptyRegistryFile();
}

void AssetRegistry::load() {
  if (!std::filesystem::exists(registryPath_)) {
    initEmptyRegistryFile();
  }

  std::vector<std::byte> buffer = readBinaryFileBytes(registryPath_);
  RegistryData data;
  if (const glz::error_ctx err = glz::read_json(data, buffer)) {
    log().fatal("Failed to read registry file: {}. error code: {}, msg: {}.", registryPath_.string(),
                std::to_underlying(err.ec), err.custom_error_message);
  }
  entries_ = std::move(data.entries);
  aliases_ = std::move(data.aliases);
}

void AssetRegistry::save() const {
  RegistryData data{.entries = entries_, .aliases = aliases_};
  const std::string serializedReg =
      glz::write_json(data).value_or(R"({"error": "Couldn't serialize the registry object."})");
  if (!writeBinaryFile(registryPath_, glz::prettify_json(serializedReg))) {
    log().fatal("Failed to save registry file: {}.", registryPath_.string());
  }
}

void AssetRegistry::addEntry(const AssetUuid& uuid, const AssetEntry&& entry) {
  entries_.emplace(uuid, std::move(entry));
}

void AssetRegistry::addAlias(const std::string& alias, const AssetUuid& uuid) {
  aliases_[alias] = uuid;
}

std::optional<AssetUuid> AssetRegistry::getUuid(const std::string& alias) const {
  const auto it = aliases_.find(alias);
  if (it == aliases_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::optional<AssetEntry> AssetRegistry::getEntry(const AssetUuid& uuid) const {
  const auto it = entries_.find(uuid);
  if (it == entries_.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool AssetRegistry::hasEntry(const AssetUuid& uuid) const {
  return entries_.contains(uuid);
}

bool AssetRegistry::hasAlias(const std::string& alias) const {
  return aliases_.contains(alias);
}

size_t AssetRegistry::getEntryCount() const {
  return entries_.size();
}

AssetBuildMode AssetRegistry::buildTypeToAssetBuildMode(BuildType buildType) const {
  switch (buildType) {
  case BuildType::Debug:
    return AssetBuildMode::Debug;
  case BuildType::Release:
  case BuildType::RelWithDebInfo:
    return AssetBuildMode::Release;
  }
  std::unreachable();
}

template <AssetDefinitionConcept TDefinition>
std::optional<TDefinition> AssetRegistry::getDefinition(const AssetUuid& uuid) const {
  using TAsset = AssetTypeFor_t<TDefinition>;
  const std::optional<const AssetEntry> entryOpt = getEntry(uuid);
  if (!entryOpt)
    return {};
  const auto& entry = entryOpt.value();

  if (entry.type != TAsset::typeEnum)
    log().fatal(
        "The asset type from the entry '{} does not match with the definition type '{}' for asset '{}'",
        glz::write_json(entry.type).value_or("unknown"), TAsset::label, uuid.to_chars());

  const AssetBuildMode mode = buildTypeToAssetBuildMode(kBuildType);

  const std::filesystem::path dstRelPath = [&entry, &mode]() {
    if (entry.dstVariantRelPaths.contains(mode))
      return entry.dstVariantRelPaths.at(mode);
    return entry.dstVariantRelPaths.at(AssetBuildMode::Any);
  }();
  const auto dstPath = getRootFolder() / dstRelPath;

  if (!std::filesystem::exists(dstPath))
    log().fatal("Asset in registry '{}' does not have a processed file at {}!", uuid.to_chars(),
                dstPath.generic_string());
  const std::vector<std::byte> defBuffer = readBinaryFileBytes(dstPath);

  TDefinition def;
  if (const glz::error_ctx err = glz::read_beve(def, defBuffer)) {
    log().warn("Failed to read asset definition from file: {}. error code: {}, msg: {}. Try reprocessing.",
               dstPath.generic_string(), std::to_underlying(err.ec), err.custom_error_message);
    return std::nullopt;
  }
  return def;
}

// Explicit template instantiations
template std::optional<asset::ShaderStageDefinition>
AssetRegistry::getDefinition<asset::ShaderStageDefinition>(const AssetUuid& uuid) const;

template std::optional<asset::GraphicsProgramDefinition>
AssetRegistry::getDefinition<asset::GraphicsProgramDefinition>(const AssetUuid& uuid) const;

} // namespace aur
