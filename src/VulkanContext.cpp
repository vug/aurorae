#if defined(CROSS_PLATFORM_SURFACE_CREATION)
  VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);
  int glfwGetError(const char** description);
#else
#include <glfw/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#endif

#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include "VulkanContext.h"
#include "utils.h"


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
    fatal("Failed to initialize Volk: {}", static_cast<int>(volkInitResult));
  {
    uint32_t instanceVersion{};
    vkEnumerateInstanceVersion(&instanceVersion);
    spdlog::trace("Vulkan Instance Version (via Volk): {}.{}.{}",
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
    vkbInstanceBuilder
      .enable_validation_layers()
      .use_default_debug_messenger();
  }
  vkb::Result<vkb::Instance> vkbInstanceResult = vkbInstanceBuilder.build();
  if (!vkbInstanceResult)
    fatal("Failed to create Vulkan instance: {}", vkbInstanceResult.error().message());
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
      fatal("Failed to create Vulkan surface: {}", errorMsg);
    else
      fatal("Failed to create Vulkan surface: Unknown error.");
  }
#else
  VkWin32SurfaceCreateInfoKHR sci{
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .hinstance = GetModuleHandle(nullptr),
    .hwnd = glfwGetWin32Window(window),
  };
  if(vkCreateWin32SurfaceKHR(vkbInstance_.instance, &sci, nullptr, &surface_) != VK_SUCCESS)
    fatal("Failed to create Win32 Vulkan surface.");
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
    .add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
    .select();
  if (!vkbPhysicalDeviceResult)
    fatal("Failed to select Vulkan Physical Device: {}", vkbPhysicalDeviceResult.error().message());    
  vkb::PhysicalDevice vkbPhysicalDevice = vkbPhysicalDeviceResult.value();

  // Create logical device
  vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice);
  auto vkbDeviceResult = deviceBuilder.build();
  if (!vkbDeviceResult)
    fatal("Failed to create Vulkan Logical Device: {}", vkbDeviceResult.error().message());
  vkbDevice_ = vkbDeviceResult.value();
  graphicsQueueFamilyIndex_ = vkbDevice_.get_queue_index(vkb::QueueType::graphics).value();
  presentQueueFamilyIndex_ = vkbDevice_.get_queue_index(vkb::QueueType::present).value(); // Often same as graphics
  spdlog::trace("graphicsQueueFamilyIndex: {}, presentQueueFamilyIndex: {}", graphicsQueueFamilyIndex_, presentQueueFamilyIndex_);
  graphicsQueue_ = vkbDevice_.get_queue(vkb::QueueType::graphics).value();
  presentQueue_ = vkbDevice_.get_queue(vkb::QueueType::present).value();  

  // Load Vulkan device-level function pointers
  volkLoadDevice(vkbDevice_.device); 
}

VulkanContext::~VulkanContext() {
  vkb::destroy_device(vkbDevice_); // Destroys VkDevice, VkCommandPools, etc.
  vkb::destroy_surface(vkbInstance_, surface_); // Destroys VkSurfaceKHR
  vkb::destroy_instance(vkbInstance_); // Destroys VkInstance, debug messenger, etc.
}

} // namespace aur