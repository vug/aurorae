#pragma once

namespace std {
// Forward declare std::string_view. Lol.
template <typename T>
struct char_traits;
template <class CharT, class Traits>
class basic_string_view;
using string_view = basic_string_view<char, std::char_traits<char>>;

enum class byte : unsigned char;
} // namespace std

#include "Utils.h"

struct GLFWwindow;

namespace aur {

class Window {
public:
  Window(u32 width, u32 height, std::string_view title);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;
  Window(Window&&) = delete;
  Window& operator=(Window&&) = delete;

  bool shouldClose() const;
  static void pollEvents();
  static void waitEvents();

  GLFWwindow* getGLFWwindow() const { return glfwWindow_; }
  bool wasResized() const { return framebufferResized_; }
  void clearResizedFlag() { framebufferResized_ = false; }
  u32 getWitdh() const { return currentWidth_; }
  u32 getHeight() const { return currentHeight_; }
  void getFramebufferSize(i32& width, i32& height) const;

private:
  static void framebufferResizeCallback(GLFWwindow* window, i32 width, i32 height);
  static void keyCallback(GLFWwindow* glfwWin, int key, int scancode, int action, int mods);

  GLFWwindow* glfwWindow_{nullptr};
  bool framebufferResized_{false};
  // Store current dimensions, updated by resize callback and constructor
  u32 currentWidth_;
  u32 currentHeight_;
};

} // namespace aur