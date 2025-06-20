#include "Window.h"

#include <string_view>

#include <glfw/glfw3.h>

#include "Logger.h"

namespace aur {

Window::Window(uint32_t width, uint32_t height, std::string_view title)
    : currentWidth_(width)
    , currentHeight_(height) {
  // GLFW initialization is expected to be done by the Application class

  // Tell GLFW not to create an OpenGL/ES context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  glfwWindow_ =
      glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.data(), nullptr, nullptr);
  if (!glfwWindow_)
    log().fatal("Failed to create GLFW window");

  glfwSetWindowUserPointer(glfwWindow_, this);
  glfwSetFramebufferSizeCallback(glfwWindow_, framebufferResizeCallback);
  log().info("Window created: {}x{}", width, height);
}

Window::~Window() {
  if (glfwWindow_) {
    glfwDestroyWindow(glfwWindow_);
    log().info("Window destroyed.");
  }
  // glfwTerminate() is handled in main
}

bool Window::shouldClose() const {
  return glfwWindowShouldClose(glfwWindow_);
}

void Window::pollEvents() {
  glfwPollEvents();
}

void Window::waitEvents() {
  glfwWaitEvents();
}

void Window::getFramebufferSize(i32& width, i32& height) const {
  glfwGetFramebufferSize(glfwWindow_, &width, &height);
}

void Window::framebufferResizeCallback(GLFWwindow* glfwWin, i32 width, i32 height) {
  auto windowInstance = static_cast<Window*>(glfwGetWindowUserPointer(glfwWin));
  if (windowInstance) {
    windowInstance->framebufferResized_ = true;
    windowInstance->currentWidth_ = static_cast<u32>(width);
    windowInstance->currentHeight_ = static_cast<u32>(height);
    log().trace("Framebuffer resized event: {}x{}", width, height);
  }
}

} // namespace aur