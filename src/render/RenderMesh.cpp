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
      Buffer vertBuf = renderer_->createBufferAndUploadData(assetHandle_->getVertices(), BufferUsage::Vertex,
                                                            assetHandle_->debugName + " Vertex Buffer");
      return vertBuf;
    }()}
    , indexBuffer_{[this]() {
      Buffer indexBuf = renderer_->createBufferAndUploadData(assetHandle_->getIndicates(), BufferUsage::Index,
                                                             assetHandle_->debugName + " Index Buffer");
      return indexBuf;
    }()}
    , drawSpans_{[this]() {
      const std::vector<DrawSpan> drawSpans =
          assetHandle_->getMaterialSpans() | std::views::transform([this](asset::MaterialSpan& matSpan) {
            const auto renderMatHandle = renderer_->uploadOrGet(matSpan.material);
            return DrawSpan{.material = renderMatHandle, .offset = matSpan.offset, .count = matSpan.count};
          }) |
          std::ranges::to<std::vector<DrawSpan>>();
      return drawSpans;
    }()} {} // render

void Mesh::draw(const glm::mat4& worldFromObject, i32 meshId) const {
  PushConstant pc{
      .worldFromObject = worldFromObject,
      .transposeInverseTransform = glm::transpose(glm::inverse(worldFromObject)),
      .meshId = meshId,
  };
  for (u32 spanIx = 0; spanIx < drawSpans_.size(); ++spanIx) {
    pc.spanId = spanIx;
    drawSpan(spanIx, pc);
  }
}

void Mesh::drawSpan(u32 spanIx, const PushConstant& pc) const {
  if (spanIx >= drawSpans_.size())
    log().fatal("spanIx {} not in range [0, {}].", spanIx, drawSpans_.size() - 1);

  const DrawSpan& dSpan = drawSpans_[spanIx];
  const Pipeline* pipeline = renderer_->createOrGetPipeline(dSpan.material->getPipelineCreateInfo());
  const PushConstantsInfo pcInfo{
      .pipelineLayout = pipeline->getPipelineLayout(),
      .stages = {ShaderStageType::Vertex},
      .sizeBytes = sizeof(pc),
      .data = &pc,
  };
  renderer_->drawIndexed(*pipeline, vertexBuffer_, indexBuffer_, &pcInfo);
}
} // namespace aur::render