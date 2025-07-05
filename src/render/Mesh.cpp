#include "Mesh.h"

#include <ranges>

#include "../Renderer.h"
#include "../asset/Mesh.h"

namespace aur::render {
Mesh::Mesh(const Renderer& renderer, Handle<asset::Mesh> asset)
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
          aMesh.getSubMeshes() | std::views::transform([](const asset::SubMesh& subMesh) {
            // TODO(vug): bring render::Material
            return DrawSpan{/*.material = someRenderMaterial, */ .offset = subMesh.offset,
                            .count = subMesh.count};
          }) |
          std::ranges::to<std::vector<DrawSpan>>();
      return drawSpans;
    }()} {} // render
} // namespace aur::render