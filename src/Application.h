#pragma once

#include "Renderer.h"
#include "Window.h"

typedef unsigned int uint32_t;

namespace aur {

class Application {
 public:
  Application(uint32_t initialWidth, uint32_t initialHeight,
              const char* appName);
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  Application(Application&&) = delete;
  Application& operator=(Application&&) = delete;

  void run();

 private:
  const char* appName_;  // Store appName
  // Order of declaration matters for construction (Window then Renderer)
  // and destruction (Renderer then Window).
  // GLFW initialization is now handled by main().
  Window window_;
  Renderer renderer_;
};

}  // namespace aur