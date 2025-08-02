#include "AssetRegistry.h"

#include "../FileIO.h"
#include "../Logger.h"
#include "GraphicsProgram.h"

#include <glaze/glaze/glaze.hpp>

namespace aur {

const std::filesystem::path AssetRegistry::kProcessedAssetsRoot{kAssetsFolder / "../processedAssets"};
const std::filesystem::path AssetRegistry::kRegistryPath{kProcessedAssetsRoot / "registry.json"};

// Create a temporary struct to match the original serialization format
struct RegistryData {
  std::unordered_map<AssetUuid, AssetEntry> entries;
  std::unordered_map<std::string, AssetUuid> aliases;
};

void AssetRegistry::initEmptyRegistryFile() {
  if (!std::filesystem::exists(filePath_)) {
    std::filesystem::create_directories(filePath_.parent_path());

    const std::string serializedReg =
        glz::write_json(RegistryData{}).value_or("{\"error\": \"Couldn't serialize the registry object.\"}");
    if (!writeBinaryFile(filePath_, glz::prettify_json(serializedReg)))
      log().fatal("Failed to initialize the registry file: {}.", filePath_.string());
  }
}

AssetRegistry::AssetRegistry(const std::filesystem::path& registryFilePath)
    : filePath_(registryFilePath) {}

void AssetRegistry::clear() {
  entries_.clear();
  aliases_.clear();

  const std::uintmax_t removedCnt = std::filesystem::remove_all(kProcessedAssetsRoot);
  log().info("Number of files and directories removed: {}.", removedCnt);

  initEmptyRegistryFile();
}

void AssetRegistry::load() {
  if (!std::filesystem::exists(filePath_)) {
    initEmptyRegistryFile();
  }

  std::vector<std::byte> buffer = readBinaryFileBytes(filePath_);
  RegistryData data;
  if (const glz::error_ctx err = glz::read_json(data, buffer)) {
    log().fatal("Failed to read registry file: {}. error code: {}, msg: {}.", filePath_.string(),
                std::to_underlying(err.ec), err.custom_error_message);
  }
  entries_ = std::move(data.entries);
  aliases_ = std::move(data.aliases);
}

void AssetRegistry::save() const {
  RegistryData data{entries_, aliases_};
  const std::string serializedReg =
      glz::write_json(data).value_or("{\"error\": \"Couldn't serialize the registry object.\"}");
  if (!writeBinaryFile(filePath_, glz::prettify_json(serializedReg))) {
    log().fatal("Failed to save registry file: {}.", filePath_.string());
  }
}

void AssetRegistry::addEntry(const AssetUuid& uuid, const AssetEntry& entry) {
  entries_[uuid] = entry;
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

template <AssetDefinition TDefinition>
std::optional<TDefinition> AssetRegistry::getDefinition(const AssetUuid& uuid) const {
  using TAsset = AssetTypeFor_t<TDefinition>;
  const auto entryIt = entries_.find(uuid);
  if (entryIt == entries_.end()) {
    log().warn("Asset entry for '{}' not found in registry.", uuid.to_chars());
    return std::nullopt;
  }
  const AssetEntry& entry = entryIt->second;

  if constexpr (std::is_same_v<TDefinition, asset::ShaderStageDefinition>) {
    if (entry.type != DefinitionType::ShaderStage)
      log().fatal("Asset '{}' is not a shader stage definition.", uuid.to_chars());
  } else if constexpr (std::is_same_v<TDefinition, asset::GraphicsProgramDefinition>) {
    if (entry.type != DefinitionType::GraphicsProgram)
      log().fatal("Asset '{}' is not a graphics program definition.", uuid.to_chars());
  } else {
    static_assert(false, "Unimplemented definition type");
  }

  const AssetBuildMode mode = buildTypeToAssetBuildMode(kBuildType);

  const std::filesystem::path dstPath = [&entry, &mode]() {
    if (entry.dstVariantPaths.contains(mode))
      return entry.dstVariantPaths.at(mode);
    return entry.dstVariantPaths.at(AssetBuildMode::Any);
  }();

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
