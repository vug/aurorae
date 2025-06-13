#pragma once

#include "Utils.h"

FORWARD_DEFINE_VK_HANDLE(VkInstance)
FORWARD_DEFINE_VK_HANDLE(VkSurfaceKHR)

struct GLFWwindow;

namespace aur {

class GlfwUtils {
public:
  // Has to be called before any other functions in Window
  static void initGLFW();
  // Call when GLFW is not needed anymore
  static void shutdownGLFW();
  // Create the surface on GLFW window
  static void createWindowSurface(VkInstance vkInstance, GLFWwindow* window, VkSurfaceKHR* surface);
};

} // namespace aur