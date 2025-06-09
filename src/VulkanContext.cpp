#include <array>

#include <volk/volk.h>

#include "GlfwUtils.h"
#include "Logger.h"
#include "Utils.h"
#include "VulkanContext.h"

struct GLFWwindow;

namespace aur {

// Validation layers are automatically enabled in debug modes
constexpr bool kEnableValidationLayers{kBuildType != BuildType::Release};
// Manually enable/disable GPU-assisted validation layers
constexpr bool kEnableGpuAssistedValidation{false};
// For performance reasons, core validation layers are disabled in GPU-assisted
// validation mode
constexpr bool kEnableCoreValidationLayers{kEnableValidationLayers &&
                                           !kEnableGpuAssistedValidation};

VulkanContext::VulkanContext(GLFWwindow* window, const char* appName) {
  // Load basic Vulkan functions such as vkEnumerateInstanceVersion,
  // vkGetInstanceProcAddr etc.
  VkResult volkInitResult = volkInitialize();
  if (volkInitResult != VK_SUCCESS)
    log().fatal("Failed to initialize Volk: {}",
                static_cast<int>(volkInitResult));
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
  vkbInstanceBuilder.set_app_name(appName)
      .enable_extension(VK_KHR_SURFACE_EXTENSION_NAME)  // not necessary
      .require_api_version(VK_MAKE_API_VERSION(0, 1, 3, 0));
  if constexpr (kEnableValidationLayers) {
    PFN_vkDebugUtilsMessengerCallbackEXT debugCallback =
        [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
           VkDebugUtilsMessageTypeFlagsEXT messageType,
           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
           [[maybe_unused]] void* pUserData) -> VkBool32 {
      const std::array<i32, 2> ignoredMessageIds = {
          0x675dc32e,  // just warns about VK_EXT_debug_utils is intended to be
                       // used in debugging only.
          0x24b5c69f,  // GPU validation:
      };
      for (const auto& msgId : ignoredMessageIds) {
        if (pCallbackData->messageIdNumber == msgId) {
          return VK_FALSE;  // Skip this message
        }
      }
      const char* severity = vkb::to_string_message_severity(messageSeverity);
      const char* type = vkb::to_string_message_type(messageType);
      switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
          log().trace("Vulkan [{}][{}]: {}", severity, type,
                      pCallbackData->pMessage);
          // can return VK_FALSE here to ignore verbose messages
          break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
          log().info("Vulkan [{}][{}]: {}", severity, type,
                     pCallbackData->pMessage);
          break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
          log().warn("Vulkan [{}][{}]: {}", severity, type,
                     pCallbackData->pMessage);
          break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
          log().error("Vulkan [{}][{}]: {}", severity, type,
                      pCallbackData->pMessage);
          break;
        default:
          log().critical("Unknown Vulkan message severity: {}",
                         static_cast<int>(messageSeverity));
      }
      return VK_FALSE;
      // Return true for validation to skip passing down the call to the driver
      // and return back VK_ERROR_VALIDATION_FAILED_EXT from VK function calls
      // return VK_TRUE;
    };
    vkbInstanceBuilder.set_debug_callback(
        debugCallback);  // vkb::default_debug_callback
  }
  if constexpr (kEnableCoreValidationLayers) {
    vkbInstanceBuilder
        // The standard Khronos ("Normal Core Check") validation layer. CPU-side
        // checks.
        .enable_validation_layers()
        .add_validation_feature_enable(
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
        .add_validation_feature_enable(
            VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
  } else if constexpr (kEnableGpuAssistedValidation) {
    // GPU-AV instruments shaders and GPU resources to detect issues that are
    // difficult to find with CPU-only validation
    vkbInstanceBuilder.enable_validation_layers()
        .add_validation_feature_disable(
            VK_VALIDATION_FEATURE_DISABLE_CORE_CHECKS_EXT)
        .add_validation_feature_enable(
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT)
        .add_validation_feature_enable(
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
        .add_validation_feature_enable(
            VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
  }

  vkb::Result<vkb::Instance> vkbInstanceResult = vkbInstanceBuilder.build();
  if (!vkbInstanceResult)
    log().fatal("Failed to create Vulkan instance: {}",
                vkbInstanceResult.error().message());
  vkbInstance_ = vkbInstanceResult.value();
  volkLoadInstance(
      vkbInstance_);  // loads Vulkan instance-level function pointers

  // Create Vulkan surface
  GlfwUtils::createWindowSurface(vkbInstance_.instance, window, &surface_);

  // Select physical device
  vkb::PhysicalDeviceSelector selector(vkbInstance_);
  VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeaturesKHR{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
  vkb::Result<vkb::PhysicalDevice> vkbPhysicalDeviceResult =
      selector
          .set_minimum_version(1, 3)  // Explicitly target Vulkan 1.3
          .set_surface(surface_)
          .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
          // vug: I guess it requires a graphics queue by default?
          .require_separate_compute_queue()
          .require_dedicated_transfer_queue()
          .require_present()
          .set_required_features(VkPhysicalDeviceFeatures{
              .samplerAnisotropy{true},
              // automatically turns on when bufferDeviceAddress enabled, but my
              // RTX4090 does not support it
              .shaderInt64{!kEnableGpuAssistedValidation},
          })
          .set_required_features_11(VkPhysicalDeviceVulkan11Features{})
          .set_required_features_12(VkPhysicalDeviceVulkan12Features{
              .uniformAndStorageBuffer8BitAccess{kEnableGpuAssistedValidation},
              .timelineSemaphore{kEnableGpuAssistedValidation},
              .bufferDeviceAddress{kEnableGpuAssistedValidation},
          })
          .set_required_features_13(VkPhysicalDeviceVulkan13Features{
              .pNext =
                  kEnableGpuAssistedValidation ? &rayQueryFeaturesKHR : nullptr,
              .dynamicRendering = true,
          })
          .add_required_extensions({
              VK_KHR_SWAPCHAIN_EXTENSION_NAME,
          })
          .add_required_extensions(
              kEnableGpuAssistedValidation
                  ? std::initializer_list<
                        const char*>{VK_KHR_RAY_QUERY_EXTENSION_NAME,
                                     VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                     VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME}
                  : std::initializer_list<const char*>{})
          .select();
  if (!vkbPhysicalDeviceResult)
    log().fatal("Failed to select Vulkan Physical Device: {}",
                vkbPhysicalDeviceResult.error().message());
  vkbPhysicalDevice_ = vkbPhysicalDeviceResult.value();

  // Create logical device
  vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice_);
  auto vkbDeviceResult = deviceBuilder.build();
  if (!vkbDeviceResult)
    log().fatal("Failed to create Vulkan Logical Device: {}",
                vkbDeviceResult.error().message());
  vkbDevice_ = vkbDeviceResult.value();
  // Load Vulkan device-level function pointers. Observed Vulkan device
  // functions are loaded without explicit call to this function. But it won't
  // hurt to call it.
  volkLoadDevice(vkbDevice_.device);

  graphicsQueueFamilyIndex_ =
      vkbDevice_.get_queue_index(vkb::QueueType::graphics).value();
  presentQueueFamilyIndex_ = vkbDevice_.get_queue_index(vkb::QueueType::present)
                                 .value();  // Often same as graphics
  log().trace("graphicsQueueFamilyIndex: {}, presentQueueFamilyIndex: {}",
              graphicsQueueFamilyIndex_, presentQueueFamilyIndex_);
  graphicsQueue_ = vkbDevice_.get_queue(vkb::QueueType::graphics).value();
  presentQueue_ = vkbDevice_.get_queue(vkb::QueueType::present).value();
}

VulkanContext::~VulkanContext() {
  vkb::destroy_device(vkbDevice_);  // Destroys VkDevice, VkCommandPools, etc.
  vkb::destroy_surface(vkbInstance_, surface_);  // Destroys VkSurfaceKHR
  vkb::destroy_instance(
      vkbInstance_);  // Destroys VkInstance, debug messenger, etc.
}

}  // namespace aur