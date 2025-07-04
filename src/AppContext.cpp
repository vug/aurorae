#include "AppContext.h"

#include "Logger.h"

namespace aur {

Renderer* AppContext::renderer_ = {};
AssetManager* AppContext::assetManager_ = {};
AssetProcessor* AppContext::assetProcessor_ = {};
bool AppContext::initialized_ = false;

void AppContext::initialize(AssetProcessor& assetProcessor, AssetManager& assetManager, Renderer& renderer) {
  assetProcessor_ = &assetProcessor;
  assetManager_ = &assetManager;
  renderer_ = &renderer;

  initialized_ = true;
  log().trace("AppContext initialized.");
}

} // namespace aur