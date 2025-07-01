#include "AppContext.h"

#include "Logger.h"

namespace aur {

Renderer* AppContext::renderer_ = {};
AssetManager* AppContext::assetManager_ = {};
bool AppContext::initialized_ = false;

void AppContext::initialize(Renderer& renderer, AssetManager& assetManager) {
  renderer_ = &renderer;
  assetManager_ = &assetManager;
  initialized_ = true;
  log().trace("AppContext initialized.");
}

} // namespace aur