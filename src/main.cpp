// TODO(vug): enable validation layers for debug builds and bring debug callback messaging
// TODO(vug): print one of each log severity level and observe colors
// TODO(vug): setup logging function at app initialization
#define CROSS_PLATFORM_SURFACE_CREATION

#include "VulkanContext.h"
#include "Swapchain.h"
#include "utils.h"

#include <spdlog/spdlog.h>
#include <glfw/glfw3.h>

int main() {
  spdlog::info("Hi!");
  spdlog::set_level(spdlog::level::debug);
  const uint32_t kWidth = 1024;
  const uint32_t kHeight = 768;
  const std::string_view kAppName = "Aurorae";

  // Initialize GLFW and create a GLFWwindow
  if (!glfwInit())
    aur::fatal("Failed to initialize GLFW");
  // Tell GLFW not to create an OpenGL/ES context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, kAppName.data(), nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    aur::fatal("Failed to create GLFW window");
  }

  // TODO(vug): put these in runMainLoop() function so that RAII objects
  // are destructed before GLFW destroys the window
  {
    aur::VulkanContext vulkanContext{window, kAppName};
    aur::Swapchain swapchain{vulkanContext, window};
  
    while (!glfwWindowShouldClose(window)) { 
      glfwPollEvents(); 
      // Draw calls using VulkanContext, and Swapchain can be made here
    }
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  spdlog::info("Bye!");
  return 0;
}
