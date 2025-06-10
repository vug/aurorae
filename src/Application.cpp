#include "Application.h"

#include "Logger.h"
#include "glfwUtils.h"

namespace aur {

Application::Initializer::Initializer() {
  // We are initializing spdlog and glfw here to reduce the complexity of the Logger and Window classes
  // and to decouple glfw init/termination from the Window class.
  logInitialize(aur::LogLevel::Trace);
  GlfwUtils::initGLFW();  
}

Application::Initializer::~Initializer() {
  GlfwUtils::shutdownGLFW();  
}

Application::Application(u32 initialWidth, u32 initialHeight,
                         const char* appName)
    : appName_(appName),
      initializer_{},  // Initialize spdlog and glfw
      window_(initialWidth, initialHeight, appName_),  // Creates window
      renderer_(window_.getGLFWwindow(), appName_, initialWidth,
                initialHeight) {
  log().info("Application starting... Build Type: {}",
             static_cast<uint8_t>(kBuildType));
  log().info("App Name: {}, Initial Dimensions: {}x{}", appName_, initialWidth,
             initialHeight);

  log().trace("Application constructed successfully.");
}

Application::~Application() {
  // Members (renderer_, window_) are automatically destructed in
  // reverse order of declaration.
  log().trace("Application shut down.");
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
      renderer_.setClearColor(0.25f, 0.25f, 0.25f);
      renderer_.drawNoVertexInput(renderer_.getCommandBuffer(), renderer_.getTrianglePipeline(), 3);
      renderer_.drawNoVertexInput(renderer_.getCommandBuffer(), renderer_.getCubePipeline(), 36);
      renderer_.endFrame();
    }
    // If beginFrame() returns false, it means it handled a situation like
    // swapchain recreation, and the loop should just continue to the next
    // iteration.
  }
  log().debug("Main loop finished.");
}

}  // namespace aur