#pragma once

#include "Renderer.h"
#include "Utils.h"
#include "Window.h"
#include "asset/AssetManager.h"

namespace aur {

class Application {
public:
  Application(u32 initialWidth, u32 initialHeight, const char* appName);
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  Application(Application&&) = delete;
  Application& operator=(Application&&) = delete;

  void run();

private:
  // Helper class to be able to initialize necessary dependency libraries before Window, Renderer
  // etc.
  class Initializer {
  public:
    Initializer();
    ~Initializer();
  };
  const char* appName_;
  Initializer initializer_;
  // Order of declaration matters for construction (Window then Renderer) and destruction (Renderer
  // then Window).
  Window window_;
  Renderer renderer_;
  AssetManager assetManager_;
};

} // namespace aur