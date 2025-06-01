// TODO(vug): introduce VMA (Vulkan Memory Allocator) for memory management
#define CROSS_PLATFORM_SURFACE_CREATION

#include "Logger.h"
#include "VulkanContext.h"
#include "Swapchain.h"

#include <glfw/glfw3.h>

int main() {
  aur::log::initialize(spdlog::level::trace);
  aur::log::at().info("Hi!");

  const uint32_t kWidth = 1024;
  const uint32_t kHeight = 768;
  const std::string_view kAppName = "Aurorae";

  // Initialize GLFW and create a GLFWwindow
  if (!glfwInit())
    aur::log::at().fatal("Failed to initialize GLFW");
  // Tell GLFW not to create an OpenGL/ES context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, kAppName.data(), nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    aur::log::at().fatal("Failed to create GLFW window");
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
  aur::log::at().info("Bye!");
  return 0;
}
