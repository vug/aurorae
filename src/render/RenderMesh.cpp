#include "Mesh.h"

#include <ranges>

#include "../Renderer.h"
#include "../asset/Mesh.h"

namespace aur::render {
Mesh::Mesh(Renderer& renderer, Handle<asset::Mesh> asset)
    : renderer_{&renderer}
    , assetHandle_{asset}
    , vertexBuffer_{[this]() {
      const asset::Mesh& aMesh = assetHandle_.get();
      Buffer vertBuf = renderer_->createBufferAndUploadData(aMesh.getVertices(), BufferUsage::Vertex,
                                                            aMesh.debugName + " Vertex Buffer");
      return vertBuf;
    }()}
    , indexBuffer_{[this]() {
      const asset::Mesh& aMesh = assetHandle_.get();
      Buffer indexBuf = renderer_->createBufferAndUploadData(aMesh.getIndicates(), BufferUsage::Index,
                                                             aMesh.debugName + " Index Buffer");
      return indexBuf;
    }()}
    , drawSpans_{[this]() {
      const asset::Mesh& aMesh = assetHandle_.get();
      const std::vector<DrawSpan> drawSpans =
          aMesh.getMaterialSpans() | std::views::transform([this](asset::MaterialSpan& matSpan) {
            const auto renderMatHandle = renderer_->uploadOrGet(matSpan.material);
            return DrawSpan{.material = renderMatHandle, .offset = matSpan.offset, .count = matSpan.count};
          }) |
          std::ranges::to<std::vector<DrawSpan>>();
      return drawSpans;
    }()} {} // render
} // namespace aur::render