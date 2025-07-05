#pragma once

#include "../Handle.h"
#include "../Resources/Buffer.h"

namespace aur {
class Renderer;
namespace asset {
class Mesh;
}
} // namespace aur

namespace aur::render {

struct DrawSpan {
  // TODO(vug): bring render::Material
  // Handle<render::Material> material;
  u32 offset;
  u32 count;
};

class Mesh {
public:
  Mesh() = default;
  Mesh(const Renderer& renderer, Handle<asset::Mesh> asset);
  ~Mesh() = default;

  Mesh(const Mesh& other) = delete;
  Mesh(Mesh&& other) noexcept = default;
  Mesh& operator=(const Mesh& other) = delete;
  Mesh& operator=(Mesh&& other) noexcept = default;

  [[nodiscard]] const Handle<asset::Mesh>& getAssetHandle() const { return assetHandle_; }
  [[nodiscard]] const Buffer& getVertexBuffer() const { return vertexBuffer_; }
  [[nodiscard]] const Buffer& getIndexBuffer() const { return indexBuffer_; }
  [[nodiscard]] const std::vector<DrawSpan>& getDrawSpans() const { return drawSpans_; }

private:
  const Renderer* renderer_{};
  Handle<asset::Mesh> assetHandle_;

  Buffer vertexBuffer_;
  Buffer indexBuffer_;
  std::vector<DrawSpan> drawSpans_;
};

} // namespace aur::render