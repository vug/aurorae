// TODO(vug): try to get rid of exceptions in favor of fatal()
// TODO(vug): go over each file and see issues with design, inclusions, methods, members etc.
// TODO(vug): introduce VMA (Vulkan Memory Allocator) for memory management
// TODO(vug): smoother resize (current vkDeviceWaitIdle in recreate causes stutter
// TODO(vug): use slang as the shader language
// TODO(vug): config for stopping the debugger at validation issues (via std::abort() or similar)
#define CROSS_PLATFORM_SURFACE_CREATION

#include <glfw/glfw3.h>     // For glfwInit/Terminate
#include <spdlog/spdlog.h>  // For spdlog::level for log_initialize

#include <cstdlib>    // For EXIT_SUCCESS, EXIT_FAILURE
#include <string_view>

#include "Application.h"  // New Application class
#include "Logger.h"

int main() {
  // We are initializing spdlog and glfw here to reduce the complexity of classes
  // and to decouple glfw initialization/termination from the Window class.
  aur::log_initialize(spdlog::level::debug);
  aur::log().trace("Logging initialized in main.");
  if (!glfwInit())
    aur::log().fatal("Failed to initialize GLFW");
  aur::log().trace("GLFW initialized in main.");

  const uint32_t kWidth = 1024;
  const uint32_t kHeight = 768;
  const std::string_view kAppName = "Aurorae";

  aur::Application app(kWidth, kHeight, kAppName);
  app.run();

  glfwTerminate();
  aur::log().trace("GLFW terminated in main.");
  return EXIT_SUCCESS;
}
