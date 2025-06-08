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
  log().debug("Application shut down.");
}

void Application::run() {
  log().debug("Starting main loop...");
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
      // beginFrame now handles clearing and starting the render pass.
      // We can set a clear color if needed, e.g., renderer_.setClearColor(...)
      // For now, it uses a default dark gray defined in Renderer.h
      renderer_.draw(renderer_.getCommandBuffer());
      renderer_.endFrame();
    }
    // If beginFrame() returns false, it means it handled a situation like
    // swapchain recreation, and the loop should just continue to the next
    // iteration.
  }
  log().debug("Main loop finished.");
  // vkDeviceWaitIdle is called in Renderer destructor
}

}  // namespace aur