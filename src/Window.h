#pragma once

#include <cstdint>
#include <string_view>

struct GLFWwindow;

namespace aur {

class Window {
 public:
  Window(uint32_t width, uint32_t height, std::string_view title);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;
  Window(Window&&) = delete;
  Window& operator=(Window&&) = delete;

  bool shouldClose() const;
  void pollEvents() const;
  void waitEvents() const;

  GLFWwindow* getGLFWwindow() const { return glfwWindow_; }
  bool wasResized() const { return framebufferResized_; }
  void clearResizedFlag() { framebufferResized_ = false; }
  void getFramebufferSize(int& width, int& height) const;

 private:
  static void framebufferResizeCallback(GLFWwindow* window, int width,
                                        int height);

  GLFWwindow* glfwWindow_{nullptr};
  bool framebufferResized_{false};
  // Store current dimensions, updated by resize callback and constructor
  uint32_t currentWidth_;
  uint32_t currentHeight_;
};

}  // namespace aur