#include "Application.h"

#include <array>      // For std::array (clear color)
#include <stdexcept>  // For std::runtime_error, std::exception

#include "Logger.h"
#include "Utils.h"  // For kBuildType

namespace aur {

Application::Application(uint32_t initialWidth, uint32_t initialHeight,
                         std::string_view appName)
    : appName_(appName),
      window_(initialWidth, initialHeight, appName_),  // Creates window
      renderer_(window_.getGLFWwindow(), appName_, initialWidth,
                initialHeight) /* Creates renderer */ {
  // Logger is expected to be initialized by main() before Application is
  // constructed.
  log().info("Application starting. Build Type: {}",
             static_cast<uint8_t>(kBuildType));
  log().info("App Name: {}, Initial Dimensions: {}x{}", appName_.c_str(),
             initialWidth, initialHeight);

  // Volk is initialized by VulkanContext constructor

  // Construction of members is done in the initializer list.
  // If any constructor throws, it will be caught by the try-catch in main().
  log().info("Application constructed successfully.");
}

Application::~Application() {
  // Members (renderer_, window_, glfwManager_) are automatically destructed in
  // reverse order of declaration. glfwManager_ destructor will call
  // glfwTerminate().
  log().info("Application shut down.");
}

void Application::run() {
  log().info("Starting main loop...");
  while (!window_.shouldClose()) {
    window_.pollEvents();

    if (window_.wasResized()) {
      int w, h;
      window_.getFramebufferSize(w, h);
      while (w == 0 || h == 0) {  // Handle minimization
        log().debug("Window minimized ({}x{}), waiting...", w, h);
        window_.getFramebufferSize(w, h);  // Re-check size
        window_.waitEvents();  // Wait for events that might restore the window
      }
      renderer_.notifyResize(static_cast<uint32_t>(w),
                             static_cast<uint32_t>(h));
      window_.clearResizedFlag();
    }

    if (renderer_.beginFrame()) {
      // Example: clear the screen. In a real app, you'd record various draw
      // commands here. The command buffer to use is internal to the renderer
      // for this simple example. For more complex scenarios, beginFrame might
      // return the command buffer. For now, Renderer::clearScreen takes the
      // command buffer and image index it needs. This clearScreen is a
      // placeholder for where your actual scene rendering would go. The current
      // Renderer::beginFrame already starts command buffer recording. The
      // current Renderer::clearScreen does the full begin/end rendering pass
      // for clearing. This needs to be adjusted if we want to insert commands
      // between begin/end rendering. For this refactor, let's assume
      // clearScreen is the *only* thing happening. The Renderer::beginFrame
      // prepares the command buffer. The Renderer::clearScreen uses that
      // command buffer. The Renderer::endFrame submits it.

      // Let's make clearScreen use the member commandBuffer_ and
      // currentImageIndex_ And it should not call vkCmdBegin/EndRendering
      // itself if beginFrame/endFrame do. For now, the provided
      // Renderer::clearScreen is self-contained for a clear. To integrate
      // properly:
      // 1. Renderer::beginFrame would do initial transition and
      // vkCmdBeginRendering.
      // 2. Application calls drawing functions (which get command buffer from
      // Renderer).
      // 3. Renderer::endFrame would do vkCmdEndRendering, final transition,
      // submit.

      // Simplified for this example: Renderer::clearScreen is a self-contained
      // operation that uses the command buffer prepared by beginFrame. Let's
      // assume beginFrame gives us a command buffer and image index. For now,
      // the current Renderer::beginFrame doesn't return them, but it sets them
      // as members. And Renderer::clearScreen is a standalone function. The
      // current structure of Renderer::beginFrame and Renderer::clearScreen
      // means clearScreen will record its own begin/end rendering.
      // This is slightly off from the typical beginFrame/draw/endFrame.
      // Let's adjust Renderer to fit the pattern better in a follow-up if
      // needed. For now, this demonstrates the loop.

      // The current Renderer::clearScreen is a full render pass.
      // Let's assume beginFrame sets up the command buffer, and clearScreen
      // uses it. The current Renderer::beginFrame already starts the command
      // buffer. The current Renderer::clearScreen does the image transitions
      // and the rendering pass.
      renderer_.clearScreen(renderer_.getCommandBuffer(),
                            renderer_.getCurrentImageIndex(),
                            {0.1f, 0.1f, 0.4f, 1.0f});
      renderer_.endFrame();
    }
    // If beginFrame() returns false, it means it handled a situation like
    // swapchain recreation, and the loop should just continue to the next
    // iteration.
  }
  log().info("Main loop finished.");
  // vkDeviceWaitIdle is called in Renderer destructor
}

}  // namespace aur