#define CROSS_PLATFORM_SURFACE_CREATION

#include "GlfwUtils.h"

// TODO(vug): see whether I can get rid off volk here
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#include <glfw/glfw3.h>
#if defined(CROSS_PLATFORM_SURFACE_CREATION)
#else
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#endif

#include "Logger.h"

namespace aur {

void GlfwUtils::initGLFW() {
  if (!glfwInit()) log().fatal("Failed to initialize GLFW!");
  log().trace("GLFW initialized.");
}

void GlfwUtils::shutdownGLFW() {
  glfwTerminate();
  log().trace("GLFW terminated.");
}

void GlfwUtils::createWindowSurface(VkInstance vkInstance, GLFWwindow* window, VkSurfaceKHR* surface) {
#if defined(CROSS_PLATFORM_SURFACE_CREATION)
  // GLFW's internal logic will use the necessary instance functions (which it
  // also loads internally, or accesses via Volk if Volk initialized first)
  // to create the platform-specific VkSurfaceKHR.
  if (glfwCreateWindowSurface(vkInstance, window, nullptr, surface) != VK_SUCCESS) {
    const char* errorMsg;
    if (glfwGetError(&errorMsg))
      log().fatal("Failed to create Vulkan surface: {}", errorMsg);
    else
      log().fatal("Failed to create Vulkan surface: Unknown error.");
  }
  log().trace("Surface created via GLFW");
#else
  VkWin32SurfaceCreateInfoKHR sci{
      .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
      .hinstance = GetModuleHandle(nullptr),
      .hwnd = glfwGetWin32Window(window),
  };
  if (vkCreateWin32SurfaceKHR(vkInstance, &sci, nullptr, surface) != VK_SUCCESS)
    log().fatal("Failed to create Win32 Vulkan surface.");
  log().trace("Surface created via vkCreateWin32SurfaceKHR.");
#endif
}

} // namespace aur