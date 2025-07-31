#include "AppContext.h"

#include "Logger.h"

namespace aur {

AssetRegistry* AppContext::assetRegistry_ = {};
AssetProcessor* AppContext::assetProcessor_ = {};
AssetManager* AppContext::assetManager_ = {};
Renderer* AppContext::renderer_ = {};
bool AppContext::initialized_ = false;

void AppContext::initialize(AssetRegistry& assetRegistry, AssetProcessor& assetProcessor,
                            AssetManager& assetManager, Renderer& renderer) {
  assetRegistry_ = &assetRegistry;
  assetProcessor_ = &assetProcessor;
  assetManager_ = &assetManager;
  renderer_ = &renderer;

  initialized_ = true;
  log().trace("AppContext initialized.");
}

} // namespace aur