#include "Application.h"

#include <glaze/glaze/json/schema.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "AppContext.h"
#include "GlfwUtils.h"
#include "Logger.h"
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
    , initializer_{} // Initializes spdlog and glfw
    , assetRegistry_{}
    , assetProcessor_{assetRegistry_}
    , assetManager_{assetRegistry_}
    , window_(initialWidth, initialHeight, appName_)
    , renderer_(window_.getGLFWwindow(), appName_, initialWidth, initialHeight) {
  log().info("Application starting... Build Type: {}", static_cast<uint8_t>(kBuildType));
  log().info("App Name: {}, Initial Dimensions: {}x{}", appName_, initialWidth, initialHeight);

  AppContext::initialize(assetRegistry_, assetProcessor_, assetManager_, renderer_);

  assetManager_.addGraphicsProgramUpdateListener([&renderer = renderer_](Handle<asset::GraphicsProgram> hnd) {
    renderer.onGraphicsProgramAssetUpdated(hnd);
  });

  log().trace("Application constructed successfully.");
}

Application::~Application() {
  // Members (renderer_, window_) are automatically destructed in reverse order of declaration.
  log().trace("Application shut down.");
}

void Application::run() {
  const bool shouldClear = true;
  if (shouldClear) {
    assetRegistry_.clear();
    assetProcessor_.processAllAssets();
  } else {
    assetRegistry_.load();
  }

  // Export schemas
  if (const auto schema = glz::write_json_schema<asset::GraphicsProgramDefinition>(); schema.has_value())
    writeBinaryFile(kAssetsFolder / "shaders/graphics_program.schema.json",
                    glz::prettify_json(schema.value()));
  if (const auto schema = glz::write_json_schema<asset::MaterialDefinition>(); schema.has_value())
    writeBinaryFile(kAssetsFolder / "materials/material.schema.json", glz::prettify_json(schema.value()));

  // "Asset Library"
  const StableId<asset::Mesh> kBoxMeshId{
      "models/glTF-Sample-Assets/BoxVertexColors/glTF/BoxVertexColors.gltf"};
  const Handle<asset::Mesh> aBoxMesh = assetManager_.load(kBoxMeshId);
  const Handle<render::Mesh> rBoxMesh = renderer_.uploadOrGet(aBoxMesh);
  const StableId<asset::Mesh> kDuckMeshId{"models/glTF-Sample-Assets/Duck/glTF/Duck.gltf"};
  const Handle<asset::Mesh> aDuckMesh = assetManager_.load(kDuckMeshId);
  const Handle<render::Mesh> rDuckMesh = renderer_.uploadOrGet(aDuckMesh);
  log().trace("Created assets...");

  log().debug("Starting main loop...");
  while (!window_.shouldClose()) {
    Window::pollEvents();

    if (window_.wasResized()) {
      int w{}, h{};
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
            glm::radians(45.0f), static_cast<f32>(window_.getWidth()) / window_.getHeight(), 0.1f, 100.0f)};
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

    struct Renderable {
      glm::mat4 worldFromObject;
      Handle<render::Mesh> rMeshHnd;
    };

    const std::vector<Renderable> renderables = {
        {
            .worldFromObject = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)),
            .rMeshHnd = rDuckMesh,
        },
        {
            .worldFromObject = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)),
            .rMeshHnd = rBoxMesh,
        }};
    // Can I make renderable const?
    for (const auto& renderable : renderables)
      renderable.rMeshHnd->draw(renderable.worldFromObject);

    renderer_.endFrame();
  }

  log().debug("Main loop finished.");
}
} // namespace aur