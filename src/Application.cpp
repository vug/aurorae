#include "Application.h"

#include <volk/volk.h>

#include "AppContext.h"
#include "GlfwUtils.h"
#include "Logger.h"
#include "Pipelines.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

Application::Application(u32 initialWidth, u32 initialHeight, const char* appName)
    : appName_(appName)
    , initializer_{} // Initialize spdlog and glfw
    , window_(initialWidth, initialHeight, appName_)
    , renderer_(window_.getGLFWwindow(), appName_, initialWidth, initialHeight) {
  log().info("Application starting... Build Type: {}", static_cast<uint8_t>(kBuildType));
  log().info("App Name: {}, Initial Dimensions: {}x{}", appName_, initialWidth, initialHeight);

  AppContext::initialize(renderer_);
  log().trace("Application constructed successfully.");
}

Application::~Application() {
  // Members (renderer_, window_) are automatically destructed in reverse order of declaration.
  log().trace("Application shut down.");
}

void Application::run() {
  Pipelines pipelines{renderer_};
  Pipeline trianglePipeline = pipelines.createTrianglePipeline();
  Pipeline cubePipeline = pipelines.createCubePipeline();

  log().debug("Starting main loop...");
  while (!window_.shouldClose()) {
    Window::pollEvents();

    if (window_.wasResized()) {
      int w, h;
      window_.getFramebufferSize(w, h);
      while (w == 0 || h == 0) { // Handle minimization
        log().debug("Window minimized ({}x{}), waiting...", w, h);
        window_.getFramebufferSize(w, h); // Re-check size
        Window::waitEvents();             // Wait for events that might restore the window
      }
      renderer_.notifyResize(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
      window_.clearResizedFlag();
    }

    if (renderer_.beginFrame()) {
      renderer_.setClearColor(0.25f, 0.25f, 0.25f);
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(renderer_.getCommandBuffer(), 0, 1, &renderer_.triangleMesh.vertexBuffer.handle,
                             &offset);
      renderer_.drawWithoutVertexInput(trianglePipeline, 3, {});

      // glm::mat4 worldFromObject = glm::rotate(glm::mat4(1.0f), 0.5f, glm::vec3(0, 1, 0));
      glm::mat4 worldFromObject = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f));
      VkPushConstantsInfoKHR /* [issue #7] */ pushConstantsInfo{
          .sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO_KHR,
          .pNext = nullptr,
          .layout = cubePipeline.pipelineLayout.handle,
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .offset = 0,
          .size = sizeof(glm::mat4),
          .pValues = glm::value_ptr(worldFromObject),
      };
      renderer_.drawWithoutVertexInput(cubePipeline, 36, &pushConstantsInfo);
      renderer_.endFrame();
    }
    // If beginFrame() returns false, it means it handled a situation like
    // swapchain recreation, and the loop should just continue to the next
    // iteration.
  }

  // TODO(vug): With proper RAII, and architecture, we shouldn't need to call wait idle for cleaning up
  // pipelines manually
  renderer_.deviceWaitIdle();
  pipelines.cleanupPipeline(trianglePipeline);
  pipelines.cleanupPipeline(cubePipeline);
  log().debug("Main loop finished.");
}
} // namespace aur