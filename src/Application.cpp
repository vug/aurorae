#include "Application.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <volk/volk.h>

#include "AppContext.h"
#include "GlfwUtils.h"
#include "Logger.h"
#include "Pipelines.h"
#include "asset/AssetManager.h"
#include "asset/Mesh.h"

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
    , renderer_(window_.getGLFWwindow(), appName_, initialWidth, initialHeight)
    , assetManager_{} {
  log().info("Application starting... Build Type: {}", static_cast<uint8_t>(kBuildType));
  log().info("App Name: {}, Initial Dimensions: {}x{}", appName_, initialWidth, initialHeight);

  AppContext::initialize(renderer_, assetManager_);
  log().trace("Application constructed successfully.");
}

Application::~Application() {
  // Members (renderer_, window_) are automatically destructed in reverse order of declaration.
  log().trace("Application shut down.");
}

void Application::run() {
  AssetManager assMan;
  // "Asset Library"
  Pipelines pipelines{renderer_};
  Pipeline unlitPipeline = pipelines.createUnlitPipeline();
  const auto modelPath =
      std::filesystem::path(kModelsFolder) / "glTF-Sample-Assets/BoxVertexColors/glTF/BoxVertexColors.gltf";
  Handle<asset::Mesh> boxMeshHandle = assMan.loadFromFile(modelPath)[0];
  asset::Mesh* mesh = assMan.get(boxMeshHandle);
  Mesh boxRenderMesh{.vertices = mesh->vertices, .indices = mesh->indices, .debugName = "GLTF Box"};
  renderer_.upload(boxRenderMesh);
  asset::Mesh triangleMesh = asset::Mesh::makeTriangle();
  Mesh triangleRenderMesh{
      .vertices = triangleMesh.vertices,
      .indices = triangleMesh.indices,
  };
  renderer_.upload(triangleRenderMesh);
  log().trace("Created assets...");

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

    // perFrame data need to be set before beginFrame().
    renderer_.perFrameData = {
        .viewFromObject = glm::lookAt(glm::vec3{-1, 2, 5}, glm::vec3{0}, glm::vec3{0, 1, 0}),
        .projectionFromView = glm::perspective(
            glm::radians(45.0f), static_cast<f32>(window_.getWitdh()) / window_.getHeight(), 0.1f, 100.0f)};
    // GLM assumes RUB coordinate system by default (when `GLM_FORCE_LEFT_HANDED is not used (which assumes
    // RUF). Vulkan's clip-space assumes RDF, i.e., the coordinate system at the end of the vertex shader is
    // RDF, z is in [0, 1] range, and y is downwards.
    // GLM's projection matrix converts RUB-space into OpenGL's clip-space, which is also RUB.
    // GLM_FORCE_DEPTH_ZERO_TO_ONE makes the range of z [0, 1]. We provide it via compile definitions in
    // CMakeLists.txt.
    // After applying the projection, we still need a conversion from RUB to RDF. This is done by negating
    // [1][1] element.
    renderer_.perFrameData.projectionFromView[1][1] *= -1;

    // If beginFrame() returns false, it means it handled a situation like
    // swapchain recreation, and the loop should just continue to the next
    // iteration.
    if (!renderer_.beginFrame())
      continue;

    renderer_.setClearColor(0.25f, 0.25f, 0.25f);

    glm::mat4 worldFromObject1 = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.5, 1.5));
    PushConstantsInfo pcInfo1{
        .pipelineLayout = unlitPipeline.pipelineLayout,
        .stages = {ShaderStage::Vertex},
        .sizeBytes = sizeof(worldFromObject1),
        .data = glm::value_ptr(worldFromObject1),
    };
    renderer_.drawIndexed(unlitPipeline, triangleRenderMesh.vertexBuffer, triangleRenderMesh.indexBuffer,
                          &pcInfo1);

    glm::mat4 worldFromObject2 = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    PushConstantsInfo pcInfo2{
        .pipelineLayout = unlitPipeline.pipelineLayout,
        .stages = {ShaderStage::Vertex},
        .sizeBytes = sizeof(worldFromObject2),
        .data = glm::value_ptr(worldFromObject2),
    };
    renderer_.drawIndexed(unlitPipeline, boxRenderMesh.vertexBuffer, boxRenderMesh.indexBuffer, &pcInfo2);
    renderer_.endFrame();
  }

  // TODO(vug): With proper RAII, and architecture, we shouldn't need to call wait idle for cleaning up
  //            pipelines manually
  renderer_.deviceWaitIdle();
  pipelines.cleanupPipeline(unlitPipeline);
  log().debug("Main loop finished.");
}
} // namespace aur