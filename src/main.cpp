// TODO(vug): go over each file and see issues with design, inclusions, methods, members etc.
// TODO(vug): create depth/stencil image of the swapchain (if needed) via VMA.
// TODO(vug): Add Renderer Vertex, Index, Uniform, Storage, Staging buffer creation via VMA methods
// TODO(vug): add Renderer Texture, Depth/Stencil Image, Offscreen Render Target creation via VMA methods
// TODO(vug): smoother resize (current vkDeviceWaitIdle in recreate causes stutter
// TODO(vug): use slang as the shader language
// TODO(vug): config for stopping the debugger at validation issues (via std::abort() or similar)
#define CROSS_PLATFORM_SURFACE_CREATION

#include <glfw/glfw3.h>     // For glfwInit/Terminate

#include "Application.h"  // New Application class
#include "Logger.h"

int main() {
  // We are initializing spdlog and glfw here to reduce the complexity of classes
  // and to decouple glfw initialization/termination from the Window class.
  aur::logInitialize(aur::LogLevel::Debug);
  aur::log().trace("Logging initialized in main.");

  if (!glfwInit())
    aur::log().fatal("Failed to initialize GLFW");
  aur::log().trace("GLFW initialized in main.");

  const uint32_t kWidth = 1024;
  const uint32_t kHeight = 768;
  const char* kAppName = "Aurorae";

  aur::Application app(kWidth, kHeight, kAppName);
  app.run();

  glfwTerminate();
  aur::log().trace("GLFW terminated in main.");
  return 0;
}
