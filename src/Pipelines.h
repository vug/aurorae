#pragma once

#include "Pipeline.h"
#include "asset/Handle.h"

namespace aur {

class Renderer;
namespace asset {
struct Shader;
}

class Pipelines {
public:
  explicit Pipelines(const Renderer& renderer);

  // "Materials" so far.
  [[nodiscard]] Pipeline createPipeline(Handle<asset::Shader> vertHandle,
                                        Handle<asset::Shader> fragHandle) const;

  void cleanupPipeline(Pipeline& pipeline) const;

private:
  const Renderer& renderer_;
};
} // namespace aur