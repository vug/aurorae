#include "Application.h"

#include <glaze/glaze/glaze.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <volk/volk.h>

#include "AppContext.h"
#include "GlfwUtils.h"
#include "Logger.h"
#include "Pipeline.h"
#include "asset/AssetManager.h"
#include "asset/Mesh.h"

#include <modern-uuid/uuid.h>

template <>
struct glz::meta<aur::DefinitionType> {
  using enum aur::DefinitionType;
  static constexpr auto value = glz::enumerate(ShaderStage, Shader, Material, Mesh);
};

namespace aur {

struct my_struct {
  int i = 287;
  double d = 3.14;
  std::string hello = "Hello World";
  std::array<uint64_t, 3> arr = {1, 2, 3};
  std::map<std::string, int> map{{"one", 1}, {"two", 2}};
};

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
    , assetProcessor_{}
    , assetManager_{}
    , window_(initialWidth, initialHeight, appName_)
    , renderer_(window_.getGLFWwindow(), appName_, initialWidth, initialHeight) {
  log().info("Application starting... Build Type: {}", static_cast<uint8_t>(kBuildType));
  log().info("App Name: {}, Initial Dimensions: {}x{}", appName_, initialWidth, initialHeight);

  AppContext::initialize(assetProcessor_, assetManager_, renderer_);

  assetManager_.addShaderUpdateListener(
      [&renderer = renderer_](Handle<asset::Shader> hnd) { renderer.onShaderAssetUpdated(hnd); });

  log().trace("Application constructed successfully.");
}

Application::~Application() {
  // Members (renderer_, window_) are automatically destructed in reverse order of declaration.
  log().trace("Application shut down.");
}

void Application::run() {
  AssetRegistry registry;

  muuid::uuid id1 = muuid::uuid::generate_sha1(muuid::uuid::namespaces::url, "shaders/unlit.vert");
  std::string alias1{"shaders/unlit"};
  AssetEntry entry1{
      .type = DefinitionType::ShaderStage,
      .srcPath = "shaders/unlit.vert",
      .dstPath = "processed/77a8b13e.vert.shaderStage.beve",
      .subAssetName = std::nullopt,
      .dependencies = std::nullopt,
  };
  registry.aliases.insert({alias1, id1.to_string()});
  registry.entries.insert({id1.to_string(), entry1});
  muuid::uuid id2 = muuid::uuid::generate_sha1(muuid::uuid::namespaces::url, "shaders/lit.vert");
  std::string alias2{"shaders/lit"};
  AssetEntry entry2{
      .type = DefinitionType::ShaderStage,
      .srcPath = "shaders/lit.vert",
      .dstPath = "processed/f4f2a7a8.vert.shaderStage.beve",
      .subAssetName = std::nullopt,
      .dependencies = std::nullopt,
  };
  registry.aliases.insert({alias2, id2.to_string()});
  registry.entries.insert({id2.to_string(), entry2});
  const std::string serializedReg = glz::write_json(registry).value_or("error");
  if (!writeBinaryFile(kAssetsFolder / "processed/registry.json", glz::prettify_json(serializedReg)))
    log().warn("Failed to write registry to file.");

  const std::optional<asset::ShaderStageDefinition> unlitVertStage =
      AssetProcessor::getDefinition<asset::ShaderStageDefinition>("shaders/unlit.vert");

  // "Asset Library"
  const std::optional<asset::ShaderDefinition> unlitShaderDefOpt =
      assetProcessor_.loadShader(std::filesystem::path{kShadersFolder} / "unlit.vert.spv",
                                 std::filesystem::path{kShadersFolder} / "unlit.frag.spv");
  if (!unlitShaderDefOpt.has_value())
    log().fatal("Failed to load unlit shader!");
  const Handle<asset::Shader> unlitAShader = assetManager_.loadShaderFromDefinition(*unlitShaderDefOpt);
  const asset::MaterialDefinition unlitMaterialDef{
      .shaderHandle = unlitAShader,
  };
  const Handle<asset::Material> unlitAMaterial = assetManager_.loadMaterialFromDefinition(unlitMaterialDef);
  const Handle<render::Material> unlitRMaterial = renderer_.uploadOrGet(unlitAMaterial);

  // Don't do pipeline creation here but at draw call
  const PipelineCreateInfo pipelineCreateInfo{
      .shader = unlitRMaterial.get().getShaderHandle(),
      .cullMode = CullMode::Back,
  };
  const Pipeline* unlitPipeline = renderer_.createOrGetPipeline(pipelineCreateInfo);
  const auto modelPath =
      std::filesystem::path(kModelsFolder) / "glTF-Sample-Assets/BoxVertexColors/glTF/BoxVertexColors.gltf";
  const std::vector<asset::MeshDefinition> meshDefs = assetProcessor_.processMeshes(modelPath);
  const Handle<asset::Mesh> boxMeshAssetHandle = assetManager_.loadMeshFromDefinition(meshDefs[0]);
  const Handle<render::Mesh> boxMeshRenderHandle = renderer_.uploadOrGet(boxMeshAssetHandle);
  const asset::MeshDefinition triangleMesh = asset::MeshDefinition::makeTriangle();
  const Handle<asset::Mesh> triangleMeshAssetHandle = assetManager_.loadMeshFromDefinition(triangleMesh);
  const Handle<render::Mesh> triangleMeshRenderHandle = renderer_.uploadOrGet(triangleMeshAssetHandle);
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

    glm::mat4 worldFromObject1 = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.5, 1.5));
    const PushConstantsInfo pcInfo1{
        .pipelineLayout = unlitPipeline->getPipelineLayout(),
        .stages = {ShaderStage::Vertex},
        .sizeBytes = sizeof(worldFromObject1),
        .data = glm::value_ptr(worldFromObject1),
    };
    renderer_.drawIndexed(*unlitPipeline, triangleMeshRenderHandle.get().getVertexBuffer(),
                          triangleMeshRenderHandle.get().getIndexBuffer(), &pcInfo1);

    glm::mat4 worldFromObject2 = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    const PushConstantsInfo pcInfo2{
        .pipelineLayout = unlitPipeline->getPipelineLayout(),
        .stages = {ShaderStage::Vertex},
        .sizeBytes = sizeof(worldFromObject2),
        .data = glm::value_ptr(worldFromObject2),
    };
    renderer_.drawIndexed(*unlitPipeline, boxMeshRenderHandle.get().getVertexBuffer(),
                          boxMeshRenderHandle.get().getIndexBuffer(), &pcInfo2);
    renderer_.endFrame();
  }

  // TODO(vug): With proper RAII, and architecture, we shouldn't need to call wait idle for cleaning up
  //            pipelines manually
  renderer_.deviceWaitIdle();
  log().debug("Main loop finished.");
}
} // namespace aur