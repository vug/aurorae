#include "Mesh.h"

#include <ranges>

#include "../Logger.h"
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

void Mesh::draw(const glm::mat4& worldFromObject) const {
  for (u32 spanIx = 0; spanIx < drawSpans_.size(); ++spanIx)
    drawSpan(spanIx, worldFromObject);
}

void Mesh::drawSpan(u32 spanIx, const glm::mat4& worldFromObject) const {
  if (spanIx >= drawSpans_.size())
    log().fatal("spanIx {} not in range [0, {}].", spanIx, drawSpans_.size() - 1);

  const DrawSpan& dSpan = drawSpans_[spanIx];
  const Pipeline* pipeline = renderer_->createOrGetPipeline(dSpan.material.get().getPipelineCreateInfo());
  const PushConstantsInfo pcInfo{
      .pipelineLayout = pipeline->getPipelineLayout(),
      .stages = {ShaderStageType::Vertex},
      .sizeBytes = sizeof(worldFromObject),
      .data = glm::value_ptr(worldFromObject),
  };
  renderer_->drawIndexed(*pipeline, vertexBuffer_, indexBuffer_, &pcInfo);
}
} // namespace aur::render