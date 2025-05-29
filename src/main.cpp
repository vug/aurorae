// Includes from dependencies
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#include <vk-bootstrap/VkBootstrap.h>
#include <spdlog/spdlog.h>
#include <glfw/glfw3.h>

int main() {
  spdlog::info("Hi!");
  spdlog::set_level(spdlog::level::debug);

  // Note: Make sure Vulkan SDK is installed
  VkResult volkInitResult = volkInitialize();
  if (volkInitResult != VK_SUCCESS) {
    spdlog::critical("Failed to initialize Volk: {}", static_cast<int>(volkInitResult));
    return -1;
  }
  uint32_t instance_version = 0;
  // Check if the function pointer is loaded
  if (vkEnumerateInstanceVersion) {
    vkEnumerateInstanceVersion(&instance_version);
    spdlog::debug("Vulkan Instance Version (via Volk): {}.{}.{}",
                 VK_API_VERSION_MAJOR(instance_version),
                 VK_API_VERSION_MINOR(instance_version),
                 VK_API_VERSION_PATCH(instance_version));              
  }  

  // Initialize GLFW
  if (!glfwInit()) {
    spdlog::critical("Failed to initialize GLFW");
    return -1;
  } 
  // Tell GLFW not to create an OpenGL/ES context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  GLFWwindow* window = glfwCreateWindow(1024, 768, "Aurorae", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    spdlog::critical("Failed to create GLFW window!");
    return -1;
  }  
  
  // Create Vulkan Instance
  vkb::InstanceBuilder vkbInstanceBuilder;
  vkbInstanceBuilder
    .set_app_name("Aurorae")
    .request_validation_layers()
    .use_default_debug_messenger();    
  vkb::Result<vkb::Instance> vkbInstanceResult = vkbInstanceBuilder.build();
  if (!vkbInstanceResult) {
    spdlog::critical("Failed to create Vulkan instance: {}", vkbInstanceResult.error().message());
    return -1;
  }
  vkb::Instance vkbInstance = vkbInstanceResult.value();
    
  // Load all global instance-level function pointers
  volkLoadInstance(vkbInstance);

  // TODO(vug): Create Vulkan surface via GLFW

  // TODO(vug): Select physical device (requires surface)

  // TODO(vug): Create logical device (requires physical device and surface)

  // TODO(vug): Load device-level function pointers

  // TODO(vug): Create swapchain (requires logical device and surface)
 
  // .enable_surface_extension();  
  // .set_minimum_version(1, 3) // Explicitly target Vulkan 1.3

  auto systemInfoRes = vkb::SystemInfo::get_system_info();
  if (vkGetInstanceProcAddr != nullptr) {
    if (systemInfoRes) {
      vkb::SystemInfo systemInfo = systemInfoRes.value();
      spdlog::debug("vk-bootstrap SystemInfo obtained. Validation Layer available? {}",
            systemInfo.validation_layers_available);
    } else {
      spdlog::error("Failed to get vk-bootstrap SystemInfo: {}",
                    systemInfoRes.error().message());
    }
  } else {
    spdlog::error("Skipping vk-bootstrap SystemInfo check as Vulkan (via Volk) is not "
      "fully initialized.");     
  }

  spdlog::info("Bye!");
  return 0;
}
