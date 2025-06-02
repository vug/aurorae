
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#if defined(CROSS_PLATFORM_SURFACE_CREATION)
VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);
int glfwGetError(const char** description);
#else
#include <glfw/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#endif

#include "VulkanContext.h"
#include "Logger.h"

namespace aur {

#ifndef NDEBUG
    constexpr bool enableValidationLayers = true;
#else
    constexpr bool enableValidationLayers = false;
#endif

VulkanContext::VulkanContext(GLFWwindow* window, std::string_view appName)  {
  // Load basic Vulkan functions such as vkEnumerateInstanceVersion, vkGetInstanceProcAddr etc.
  VkResult volkInitResult = volkInitialize();
  if (volkInitResult != VK_SUCCESS)
    log().fatal("Failed to initialize Volk: {}", static_cast<int>(volkInitResult));
  {
    uint32_t instanceVersion{};
    vkEnumerateInstanceVersion(&instanceVersion);
    log().trace("Vulkan Instance Version (via Volk): {}.{}.{}",
                  VK_API_VERSION_MAJOR(instanceVersion),
                  VK_API_VERSION_MINOR(instanceVersion),
                  VK_API_VERSION_PATCH(instanceVersion));   
  }

  // Create Vulkan instance
  vkb::InstanceBuilder vkbInstanceBuilder;
  vkbInstanceBuilder
    .set_app_name(appName.data())
    .enable_extension(VK_KHR_SURFACE_EXTENSION_NAME) // not necessary
    .require_api_version(VK_MAKE_API_VERSION(0, 1, 3, 0));
  if (enableValidationLayers) {
    PFN_vkDebugUtilsMessengerCallbackEXT debugCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData) -> VkBool32 {
      const char* severity = vkb::to_string_message_severity(messageSeverity);
      const char* type = vkb::to_string_message_type(messageType);      
      switch(messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
          log().trace("Vulkan [{}][{}]: {}", severity, type, pCallbackData->pMessage);
          // can return VK_FALSE here to ignore verbose messages
          break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
          log().info("Vulkan [{}][{}]: {}", severity, type, pCallbackData->pMessage);
          break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
          log().warn("Vulkan [{}][{}]: {}", severity, type, pCallbackData->pMessage);
          break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
          log().error("Vulkan [{}][{}]: {}", severity, type, pCallbackData->pMessage);
          break;
        default:
          log().critical("Unknown Vulkan message severity: {}", static_cast<int>(messageSeverity));
      }
      return VK_FALSE;
      // Return true for validation to skip passing down the call to the driver
      // and return back VK_ERROR_VALIDATION_FAILED_EXT from VK function calls     
      // return VK_TRUE;
    };
    vkbInstanceBuilder
      .enable_validation_layers()
      .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
      .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT)
      .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT)
      // .set_debug_callback(vkb::default_debug_callback)
      .set_debug_callback(debugCallback);
  }
  vkb::Result<vkb::Instance> vkbInstanceResult = vkbInstanceBuilder.build();
  if (!vkbInstanceResult)
    log().fatal("Failed to create Vulkan instance: {}", vkbInstanceResult.error().message());
  vkbInstance_ = vkbInstanceResult.value();
  volkLoadInstance(vkbInstance_); // loads Vulkan instance-level function pointers

  // Create Vulkan surface
#if defined(CROSS_PLATFORM_SURFACE_CREATION)
  // GLFW's internal logic will use the necessary instance functions (which it
  // also loads internally, or accesses via Volk if Volk initialized first)
  // to create the platform-specific VkSurfaceKHR.
  if (glfwCreateWindowSurface(vkbInstance_.instance, window, nullptr, &surface_) != VK_SUCCESS) {
    const char* errorMsg;
    if(glfwGetError(&errorMsg))
      log().fatal("Failed to create Vulkan surface: {}", errorMsg);
    else
      log().fatal("Failed to create Vulkan surface: Unknown error.");
  }
#else
  VkWin32SurfaceCreateInfoKHR sci{
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .hinstance = GetModuleHandle(nullptr),
    .hwnd = glfwGetWin32Window(window),
  };
  if(vkCreateWin32SurfaceKHR(vkbInstance_.instance, &sci, nullptr, &surface_) != VK_SUCCESS)
    log().fatal("Failed to create Win32 Vulkan surface.");
#endif

  // Select physical device
  vkb::PhysicalDeviceSelector selector(vkbInstance_);
  vkb::Result<vkb::PhysicalDevice> vkbPhysicalDeviceResult = selector
    .set_minimum_version(1, 3) // Explicitly target Vulkan 1.3
    .set_surface(surface_)
    .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
    // vug: I guess it requires a graphics queue by default?
    .require_separate_compute_queue()
    .require_dedicated_transfer_queue()
    .require_present()
    .set_required_features(VkPhysicalDeviceFeatures{
      .samplerAnisotropy = true
    })
    .set_required_features_11(VkPhysicalDeviceVulkan11Features{})
    .set_required_features_12(VkPhysicalDeviceVulkan12Features{})
    .set_required_features_13(VkPhysicalDeviceVulkan13Features{
      .dynamicRendering = true
    })
    .add_required_extensions({
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    })
    .select();
  if (!vkbPhysicalDeviceResult)
    log().fatal("Failed to select Vulkan Physical Device: {}", vkbPhysicalDeviceResult.error().message());    
  vkbPhysicalDevice_ = vkbPhysicalDeviceResult.value();

  // Create logical device
  vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice_);
  auto vkbDeviceResult = deviceBuilder.build();
  if (!vkbDeviceResult)
    log().fatal("Failed to create Vulkan Logical Device: {}", vkbDeviceResult.error().message());
  vkbDevice_ = vkbDeviceResult.value();
  // Load Vulkan device-level function pointers. Observed Vulkan device functions are loaded
  // without explicit call to this function. But it won't hurt to call it.
  volkLoadDevice(vkbDevice_.device);
  
  graphicsQueueFamilyIndex_ = vkbDevice_.get_queue_index(vkb::QueueType::graphics).value();
  presentQueueFamilyIndex_ = vkbDevice_.get_queue_index(vkb::QueueType::present).value(); // Often same as graphics
  log().trace("graphicsQueueFamilyIndex: {}, presentQueueFamilyIndex: {}", graphicsQueueFamilyIndex_, presentQueueFamilyIndex_);
  graphicsQueue_ = vkbDevice_.get_queue(vkb::QueueType::graphics).value();
  presentQueue_ = vkbDevice_.get_queue(vkb::QueueType::present).value();  

}

VulkanContext::~VulkanContext() {
  vkb::destroy_device(vkbDevice_); // Destroys VkDevice, VkCommandPools, etc.
  vkb::destroy_surface(vkbInstance_, surface_); // Destroys VkSurfaceKHR
  vkb::destroy_instance(vkbInstance_); // Destroys VkInstance, debug messenger, etc.
}

} // namespace aur