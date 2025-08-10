#pragma once

#include "../Handle.h"
#include "../Resources/Buffer.h"
#include "../render/Material.h"

namespace aur {
class Renderer;
namespace asset {
class Mesh;
}
} // namespace aur

namespace aur::render {

struct DrawSpan {
  Handle<render::Material> material;
  u32 offset;
  u32 count;
};

class Mesh {
public:
  Mesh() = default;
  Mesh(Renderer& renderer, Handle<asset::Mesh> asset);
  ~Mesh() = default;

  Mesh(const Mesh& other) = delete;
  Mesh(Mesh&& other) noexcept = default;
  Mesh& operator=(const Mesh& other) = delete;
  Mesh& operator=(Mesh&& other) noexcept = default;

  void draw(const glm::mat4& worldFromObject) const;
  void drawSpan(u32 spanIx, const glm::mat4& worldFromObject) const;

  [[nodiscard]] const Handle<asset::Mesh>& getAssetHandle() const { return assetHandle_; }
  [[nodiscard]] const Buffer& getVertexBuffer() const { return vertexBuffer_; }
  [[nodiscard]] const Buffer& getIndexBuffer() const { return indexBuffer_; }
  [[nodiscard]] const std::vector<DrawSpan>& getDrawSpans() const { return drawSpans_; }

private:
  Renderer* renderer_{};
  Handle<asset::Mesh> assetHandle_;

  Buffer vertexBuffer_;
  Buffer indexBuffer_;
  std::vector<DrawSpan> drawSpans_;
};

} // namespace aur::render