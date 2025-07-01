#pragma once
#include <type_traits>

#include "Logger.h"

namespace aur {

class Application;
class AssetManager;
class Renderer;

class AppContext {
public:
  template <typename TService>
  static TService& get() {
    return getImpl<TService>();
  }
  template <typename TService>
  static const TService& getConst() {
    return getImpl<TService>();
  }

private:
  friend class Application;

  // initialize is meant to be called by Application
  static void initialize(Renderer& renderer, AssetManager& assetManager);

  template <typename TService>
  static TService& getImpl() {
    if (!initialized_)
      log().fatal("get was called before AppContext was initialized.");
    if constexpr (std::is_same_v<TService, Renderer>) {
      return *renderer_;
    } else if constexpr (std::is_same_v<TService, AssetManager>) {
      return *assetManager_;
    } else {
      static_assert(false, "Getting TService not implemented");
      std::unreachable();
    }
  }

  static Renderer* renderer_;
  static AssetManager* assetManager_;
  static bool initialized_;
};

} // namespace aur