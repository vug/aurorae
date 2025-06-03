#pragma once

#include <cstdint>  // For uint32_t
#include <memory>   // For std::unique_ptr
#include <string_view>

#include "Renderer.h"  // Needs full definition for unique_ptr
#include "Window.h"    // Needs full definition for unique_ptr

namespace aur {

class Application {
 public:
  Application(uint32_t initialWidth, uint32_t initialHeight,
              std::string_view appName);
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;
  Application(Application&&) = delete;
  Application& operator=(Application&&) = delete;

  void run();

 private:
  const std::string appName_;  // Store appName
  // Order of declaration matters for construction (Window then Renderer)
  // and destruction (Renderer then Window).
  // GLFW initialization is now handled by main().
  Window window_;
  Renderer renderer_;
};

}  // namespace aur