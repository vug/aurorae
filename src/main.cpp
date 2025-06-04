// TODO(vug): try to get rid of exceptions in favor of fatal()
// TODO(vug): go over each file and see issues with design, inclusions, methods, members etc.
// TODO(vug): fix remaining core validation issues
// TODO(vug): fix remaining GPU-assisted validation issues
// TODO(vug): introduce VMA (Vulkan Memory Allocator) for memory management
// TODO(vug): smoother resize (current vkDeviceWaitIdle in recreate causes
// stutter)
#define CROSS_PLATFORM_SURFACE_CREATION

#include <glfw/glfw3.h>     // For glfwInit/Terminate
#include <spdlog/spdlog.h>  // For spdlog::level for log_initialize

#include <cstdlib>    // For EXIT_SUCCESS, EXIT_FAILURE
#include <exception>  // For std::exception
#include <stdexcept>  // For std::runtime_error
#include <string_view>

#include "Application.h"  // New Application class
#include "Logger.h"

int main() {
  // We are initializing spdlog and glfw here to reduce the complexity of classes
  // and to decouple glfw initialization/termination from the Window class.
  aur::log_initialize(spdlog::level::debug);
  if (!glfwInit())
    aur::log().fatal("Failed to initialize GLFW");
  aur::log().trace("GLFW initialized in main.");

  const uint32_t kWidth = 1024;
  const uint32_t kHeight = 768;
  const std::string_view kAppName = "Aurorae";
  try {
    aur::Application app(kWidth, kHeight, kAppName);
    app.run();
  } catch (const std::exception& e) {
    aur::log().critical("Unhandled exception in main: {}", e.what());
    glfwTerminate();
    aur::log().info("GLFW terminated due to exception.");
    return EXIT_FAILURE;
  }

  glfwTerminate();
  aur::log().trace("GLFW terminated in main.");
  return EXIT_SUCCESS;
}
