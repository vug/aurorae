#pragma once

#include "../Handle.h"
#include "../asset/Material.h"

namespace aur {
class Renderer;
class Pipeline;
} // namespace aur

namespace aur::render {
class GraphicsProgram;

class Material {
public:
  Material() = default;
  Material(Renderer& renderer, Handle<asset::Material> asset);
  ~Material() = default;

  Material(const Material& other) = delete;
  Material(Material&& other) noexcept = default;
  Material& operator=(const Material& other) = delete;
  Material& operator=(Material&& other) noexcept = default;

  [[nodiscard]] inline Handle<render::GraphicsProgram> getGraphicsProgramHandle() const {
    return graphicsProgramHandle_;
  }

private:
  Renderer* renderer_{};
  Handle<asset::Material> assetHandle_;
  Handle<render::GraphicsProgram> graphicsProgramHandle_;
  const Pipeline* pipeline_{};
};

} // namespace aur::render