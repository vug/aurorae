#pragma once

#include <unordered_map>
#include <vector>

#include "Handle.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"

#include <filesystem>

namespace aur {

namespace asset {
struct Mesh;
} // namespace asset

class AssetManager {
public:
  Handle<asset::Shader> loadShaderFromFile(const std::filesystem::path& path);
  std::vector<Handle<asset::Mesh>> loadMeshFromFile(const std::filesystem::path& path);
  Handle<asset::Mesh> loadExistingMesh(const asset::Mesh& mesh);

  // asset::Texture* get(Handle<asset::Texture> handle);
  inline const asset::Shader* get(Handle<asset::Shader> handle) const { return &shaders_.at(handle); }
  inline const asset::Material* get(Handle<asset::Material> handle) const { return &materials_.at(handle); }
  inline const asset::Mesh* get(Handle<asset::Mesh> handle) const { return &meshes_.at(handle); };

private:
  // The manager OWNS the actual asset data in vectors.
  // std::vector<asset::Texture> textures_
  std::vector<asset::Shader> shaders_;
  std::vector<asset::Material> materials_;
  std::vector<asset::Mesh> meshes_;

  // TODO(vug): implement caching
  // cache shaders
  std::unordered_map<std::string, Handle<asset::Shader>> loadedShaders_;
  // Caching to de-duplicate materials and textures
  std::unordered_map<std::string, Handle<asset::Material>> loadedMaterials_;
  // Caching to prevent loading the same file twice
  std::unordered_map<std::filesystem::path, std::vector<Handle<asset::Mesh>>> loadedModels_;
  // std::unordered_map<std::filesystem::path, Handle<asset::Texture>> loadedTextures_;
};

} // namespace aur