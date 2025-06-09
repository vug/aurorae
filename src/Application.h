#pragma once

#include "Utils.h"
#include "Renderer.h"
#include "Window.h"

namespace aur {

class Application {
 public:
  Application(u32 initialWidth, u32 initialHeight,
              const char* appName);
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  Application(Application&&) = delete;
  Application& operator=(Application&&) = delete;

  void run();

 private:
  // Helper class to be able to initialize necessary dependency libraries before Window, Renderer etc.
  class Initializer {
    public:
      Initializer();
      ~Initializer();
  };
  const char* appName_;  // Store appName
  Initializer initializer_;
  // Order of declaration matters for construction (Window then Renderer)
  // and destruction (Renderer then Window).
  // GLFW initialization is now handled by main().
  Window window_;
  Renderer renderer_;
};

}  // namespace aur