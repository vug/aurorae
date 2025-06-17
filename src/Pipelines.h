#pragma once

#include "Pipeline.h"

namespace aur {

class Renderer;

class Pipelines {
public:
  explicit Pipelines(const Renderer& renderer);

  // "Materials" so far.
  [[nodiscard]] Pipeline createTrianglePipeline() const;
  [[nodiscard]] Pipeline createCubePipeline() const;

  void cleanupPipeline(Pipeline& pipeline) const;

private:
  const Renderer& renderer_;
};
} // namespace aur