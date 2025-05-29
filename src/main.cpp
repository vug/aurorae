#define CROSS_PLATFORM_SURFACE_CREATION
// Includes from dependencies
#if !defined(CROSS_PLATFORM_SURFACE_CREATION)
  #define VK_USE_PLATFORM_WIN32_KHR
#endif
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#include <vk-bootstrap/VkBootstrap.h>
#include <spdlog/spdlog.h>
#include <glfw/glfw3.h>
#if !defined(CROSS_PLATFORM_SURFACE_CREATION)
  #define GLFW_EXPOSE_NATIVE_WIN32
  #define GLFW_NATIVE_INCLUDE_NONE
  #include <glfw/glfw3native.h>
  #define _AMD64_
  #include <libloaderapi.h>
#endif

// TODO(vug): abstract Vulkan initialization and objects in VulkanContext class
// See "Forward declaration of Swapchain class if it's separate"

int main() {
  spdlog::info("Hi!");
  spdlog::set_level(spdlog::level::debug);
  const uint32_t kWidth = 1024;
  const uint32_t kHeight = 768;

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
  if (vkGetInstanceProcAddr) {
    vkb::SystemInfo systemInfo = vkb::SystemInfo::get_system_info().value();
    spdlog::debug("vk-bootstrap SystemInfo obtained. Validation Layer available? {}",
          systemInfo.validation_layers_available);  
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

  // Initialize GLFW
  if (!glfwInit()) {
    spdlog::critical("Failed to initialize GLFW");
    return -1;
  } 
  // Tell GLFW not to create an OpenGL/ES context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  GLFWwindow* window = glfwCreateWindow(kWidth, kHeight, "Aurorae", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    spdlog::critical("Failed to create GLFW window!");
    return -1;
  }  
  
  // Create Vulkan surface
  VkSurfaceKHR surface;
#if defined(CROSS_PLATFORM_SURFACE_CREATION)
  // GLFW's internal logic will use the necessary instance functions (which it
  // also loads internally, or accesses via Volk if Volk initialized first)
  // to create the platform-specific VkSurfaceKHR.
  if (glfwCreateWindowSurface(vkbInstance.instance, window, nullptr, &surface) != VK_SUCCESS) {
    const char* errorMsg;
    if(glfwGetError(&errorMsg))
      spdlog::critical("Failed to create Vulkan surface: {}", errorMsg);
    else
      spdlog::critical("Failed to create Vulkan surface: Unknown error.");
    return -1;
  }
#else
  VkWin32SurfaceCreateInfoKHR sci{
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .hinstance = GetModuleHandleA(nullptr),
    .hwnd = glfwGetWin32Window(window),
  };
  if(vkCreateWin32SurfaceKHR(vkbInstance.instance, &sci, nullptr, &surface) != VK_SUCCESS) {
    spdlog::critical("Failed to create Win32 Vulkan surface.");
    return -1;
  }
#endif
  
  // Select physical device
  vkb::PhysicalDeviceSelector selector(vkbInstance);
  vkb::Result<vkb::PhysicalDevice> vkbPhysicalDeviceResult = selector
    .set_minimum_version(1, 3) // Explicitly target Vulkan 1.3
    .set_surface(surface)
    .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
    // vug: I guess it requires a graphics queue by default?
    .require_separate_compute_queue()
    .require_dedicated_transfer_queue()
    .require_present()
    .set_required_features(VkPhysicalDeviceFeatures{
      .samplerAnisotropy = true}
    )
    .set_required_features_11(VkPhysicalDeviceVulkan11Features{})
    .set_required_features_12(VkPhysicalDeviceVulkan12Features{})
    .set_required_features_13(VkPhysicalDeviceVulkan13Features{})
    .add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
    .select();
  if (!vkbPhysicalDeviceResult) {
    spdlog::critical("Failed to select Vulkan Physical Device: {}", vkbPhysicalDeviceResult.error().message());
    return -1;
  }
  vkb::PhysicalDevice vkbPhysicalDevice = vkbPhysicalDeviceResult.value();

  // Create logical device
  vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice);
  auto vkbDeviceResult = deviceBuilder.build();
  if (!vkbDeviceResult) {
    spdlog::critical("Failed to create Vulkan Logical Device: {}", vkbDeviceResult.error().message());
    return -1;
  }
  vkb::Device vkbDevice = vkbDeviceResult.value();
  uint32_t graphicsQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
  uint32_t presentQueueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::present).value(); // Often same as graphics
  spdlog::debug("graphicsQueueFamilyIndex: {}, presentQueueFamilyIndex: {}", graphicsQueueFamilyIndex, presentQueueFamilyIndex);
  [[maybe_unused]] VkQueue graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
  [[maybe_unused]] VkQueue presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();  

  // Load device-level function pointers
  volkLoadDevice(vkbDevice.device);

  // Create swapchain
  vkb::SwapchainBuilder swapchainBuilder(vkbDevice, surface);
  auto vkbSwapchainResult = swapchainBuilder
      .set_old_swapchain(VK_NULL_HANDLE) // For first creation, no old swapchain
      .set_desired_extent(kWidth, kHeight)
      // default surface format
      .set_desired_format(VkSurfaceFormatKHR{
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
      })
      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // VSync
      .build();
  if (!vkbSwapchainResult) {
    spdlog::critical("Failed to create Vulkan Swapchain: {}", vkbSwapchainResult.error().message());
    return -1;
  }
  vkb::Swapchain vkbSwapchain = vkbSwapchainResult.value();

  // Main Loop
  while (!glfwWindowShouldClose(window)) { 
    glfwPollEvents(); 
    /* Render frame */ 
  }

  // Cleanup
  vkb::destroy_swapchain(vkbSwapchain); // Destroys VkSwapchainKHR, VkImageViews, etc.
  vkb::destroy_device(vkbDevice); // Destroys VkDevice, VkCommandPools, etc.
  vkb::destroy_surface(vkbInstance, surface); // Destroys VkSurfaceKHR
  vkb::destroy_instance(vkbInstance); // Destroys VkInstance, debug messenger, etc.
  glfwDestroyWindow(window);
  glfwTerminate();

  spdlog::info("Bye!");
  return 0;
}
