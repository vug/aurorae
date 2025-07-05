#pragma once

#include "../Handle.h"
#include "../asset/Material.h"

namespace aur {
class Renderer;
}

namespace aur::render {

class Material {
public:
  Material() = default;
  Material(const Renderer& renderer, Handle<asset::Material> assetHandle);
  ~Material() = default;

  Material(const Material& other) = delete;
  Material(Material&& other) noexcept = default;
  Material& operator=(const Material& other) = delete;
  Material& operator=(Material&& other) noexcept = default;

private:
  const Renderer* renderer_{};
  Handle<asset::Material> assetHandle_;
};

} // namespace aur::render