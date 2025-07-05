#include "Window.h"

#include <string_view>

#include <glfw/glfw3.h>

#include "Logger.h"

namespace aur {

Window::Window(u32 width, u32 height, std::string_view title)
    : currentWidth_(width)
    , currentHeight_(height) {
  // Tell GLFW not to create an OpenGL/ES context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  // glfwWindow_ initialization via CreateWindow cannot be done via constructor member initializer
  // and is expected to be done by the Application class
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  glfwWindow_ =
      glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.data(), nullptr, nullptr);
  if (!glfwWindow_)
    log().fatal("Failed to create GLFW window");

  glfwSetWindowUserPointer(glfwWindow_, this);
  glfwSetFramebufferSizeCallback(glfwWindow_, framebufferResizeCallback);
  glfwSetKeyCallback(glfwWindow_, keyCallback); // Register the key callback

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
  if (auto windowInstance = static_cast<Window*>(glfwGetWindowUserPointer(glfwWin))) {
    windowInstance->framebufferResized_ = true;
    windowInstance->currentWidth_ = static_cast<u32>(width);
    windowInstance->currentHeight_ = static_cast<u32>(height);
    log().trace("Framebuffer resized event: {}x{}", width, height);
  }
}

void Window::keyCallback(GLFWwindow* glfwWin, int key, [[maybe_unused]] int scancode, int action, int mods) {
  if (key == GLFW_KEY_Q && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
    glfwSetWindowShouldClose(glfwWin, GLFW_TRUE);
  }
}

} // namespace aur