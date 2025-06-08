#include "Window.h"

#include <glfw/glfw3.h>

#include <stdexcept>  // For std::runtime_error

#include "Logger.h"

namespace aur {

void Window::initGLFW() {
  if (!glfwInit())
    log().fatal("Failed to initialize GLFW!");
  log().trace("GLFW initialized.");
}

void Window::shutdownGLFW() {
  glfwTerminate();
  log().trace("GLFW terminated.");
}

Window::Window(uint32_t width, uint32_t height, std::string_view title)
    : currentWidth_(width), currentHeight_(height) {
  // GLFW initialization is expected to be done by the Application class
  glfwWindowHint(GLFW_CLIENT_API,
                 GLFW_NO_API);  // Tell GLFW not to create an OpenGL/ES context
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  glfwWindow_ =
      glfwCreateWindow(static_cast<int>(width), static_cast<int>(height),
                       title.data(), nullptr, nullptr);
  if (!glfwWindow_) {
    // glfwTerminate() will be called by Application
    throw std::runtime_error("Failed to create GLFW window");
  }

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

bool Window::shouldClose() const { return glfwWindowShouldClose(glfwWindow_); }

void Window::pollEvents() const { glfwPollEvents(); }

void Window::waitEvents() const { glfwWaitEvents(); }

void Window::getFramebufferSize(int& width, int& height) const {
  glfwGetFramebufferSize(glfwWindow_, &width, &height);
}

void Window::framebufferResizeCallback(GLFWwindow* glfwWin, int width,
                                       int height) {
  auto windowInstance = static_cast<Window*>(glfwGetWindowUserPointer(glfwWin));
  if (windowInstance) {
    windowInstance->framebufferResized_ = true;
    windowInstance->currentWidth_ = static_cast<uint32_t>(width);
    windowInstance->currentHeight_ = static_cast<uint32_t>(height);
    // log().trace("Framebuffer resized event: {}x{}", width, height); //
    // Optional: for debugging
  }
}

}  // namespace aur