#pragma once

#include <unordered_map>
#include <vector>

#include "Handle.h"
#include "Material.h"
#include "Mesh.h"

#include <filesystem>

namespace aur {

namespace asset {
struct Mesh;
} // namespace asset

class AssetManager {
public:
  std::vector<Handle<asset::Mesh>> loadFromFile(const std::filesystem::path& path);

  inline asset::Mesh* get(Handle<asset::Mesh> handle) { return &meshes_.at(handle); };
  inline asset::Material* get(Handle<asset::Material> handle) {
    return &materials_.at(handle);
  } // asset::Texture* get(Handle<asset::Texture> handle);

private:
  // The manager OWNS the actual asset data in vectors.
  std::vector<asset::Mesh> meshes_;
  std::vector<asset::Material> materials_;
  // std::vector<asset::Texture> textures_;

  // TODO(vug): implement caching
  // Caching to prevent loading the same file twice
  std::unordered_map<std::filesystem::path, std::vector<Handle<asset::Mesh>>> loadedModels_;
  // Caching to de-duplicate materials and textures
  std::unordered_map<std::string, Handle<asset::Material>> loadedMaterials_;
  // std::unordered_map<std::filesystem::path, Handle<asset::Texture>> loadedTextures_;
};

} // namespace aur