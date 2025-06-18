#include "AppContext.h"

#include "Logger.h"
#include "Renderer.h"

namespace aur {
Renderer* AppContext::renderer_ = {};

void AppContext::initialize(Renderer& renderer) {
  renderer_ = &renderer;
}

const Renderer& AppContext::getRenderer() {
  if (!renderer_)
    log().fatal("getRenderer() is called before AppContext was initialized.");
  return *renderer_;
}
} // namespace aur