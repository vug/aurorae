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
constexpr bool kEnableCoreValidationLayers{kEnableValidationLayers && !kEnableGpuAssistedValidation};
constexpr bool kDebugBreakAtValidationErrors{true};

VkBool32 VKAPI_PTR debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                                 const VkDebugUtilsMessengerCallbackDataEXT*, void*);

VulkanContext::VulkanContext(GLFWwindow* window, const char* appName) {
  // Load basic Vulkan functions such as vkEnumerateInstanceVersion,
  // vkGetInstanceProcAddr etc.
  VK(volkInitialize());
  {
    uint32_t instanceVersion{};
    vkEnumerateInstanceVersion(&instanceVersion);
    log().trace("Vulkan Instance Version (via Volk): {}.{}.{}", VK_API_VERSION_MAJOR(instanceVersion),
                VK_API_VERSION_MINOR(instanceVersion), VK_API_VERSION_PATCH(instanceVersion));
  }

  // Create Vulkan instance
  vkb::InstanceBuilder vkbInstanceBuilder;
  vkbInstanceBuilder.set_app_name(appName)
      .enable_extension(VK_KHR_SURFACE_EXTENSION_NAME) // not necessary
      .require_api_version(VK_MAKE_API_VERSION(0, 1, 4, 313));
  if constexpr (kEnableValidationLayers) {
    vkbInstanceBuilder.set_debug_callback(debugCallback); // vkb::default_debug_callback
  }
  if constexpr (kEnableCoreValidationLayers) {
    vkbInstanceBuilder
        // The standard Khronos ("Normal Core Check") validation layer. CPU-side
        // checks.
        .enable_validation_layers()
        .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
        .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
  } else if constexpr (kEnableGpuAssistedValidation) {
    // GPU-AV instruments shaders and GPU resources to detect issues that are
    // challenging to find with CPU-only validation
    vkbInstanceBuilder.enable_validation_layers()
        .add_validation_feature_disable(VK_VALIDATION_FEATURE_DISABLE_CORE_CHECKS_EXT)
        .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT)
        .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
        .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
  }

  vkb::Result<vkb::Instance> vkbInstanceResult = vkbInstanceBuilder.build();
  if (!vkbInstanceResult)
    log().fatal("Failed to create Vulkan instance: {}", vkbInstanceResult.error().message());
  vkbInstance_ = vkbInstanceResult.value();
  volkLoadInstance(vkbInstance_); // loads Vulkan instance-level function pointers

  // Create Vulkan surface
  GlfwUtils::createWindowSurface(vkbInstance_.instance, window, &vkbInstance_.surface);

  // Select physical device
  vkb::PhysicalDeviceSelector selector(vkbInstance_);
  VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeaturesKHR{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
  vkb::Result<vkb::PhysicalDevice> vkbPhysicalDeviceResult =
      selector
          .set_minimum_version(1, 4) // Explicitly target Vulkan 1.3
          .set_surface(vkbInstance_.surface)
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
              .scalarBlockLayout{VK_TRUE},
              .timelineSemaphore{kEnableGpuAssistedValidation},
              .bufferDeviceAddress{kEnableGpuAssistedValidation},
          })
          .set_required_features_13(VkPhysicalDeviceVulkan13Features{
              .pNext = kEnableGpuAssistedValidation ? &rayQueryFeaturesKHR : nullptr,
              .dynamicRendering = true,
          })
          .add_required_extensions({
              VK_KHR_SWAPCHAIN_EXTENSION_NAME,
              // Needed for vkCmdPushConstants2KHR [issue #7]. Ideally, this shouldn't be needed because that
              // function is part of Vulkan 1.4, but apparently my laptop graphics drivers do not support it
              // yet.
              VK_KHR_MAINTENANCE_6_EXTENSION_NAME,
          })
          .add_required_extensions(
              kEnableGpuAssistedValidation
                  ? std::initializer_list<const char*>{VK_KHR_RAY_QUERY_EXTENSION_NAME,
                                                       VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                       VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME}
                  : std::initializer_list<const char*>{})
          .select();
  if (!vkbPhysicalDeviceResult)
    log().fatal("Failed to select Vulkan Physical Device: {}", vkbPhysicalDeviceResult.error().message());
  vkbPhysicalDevice_ = vkbPhysicalDeviceResult.value();
  log().trace("Selected Physical Device Name: {}", vkbPhysicalDevice_.properties.deviceName);
  log().trace("Selected Physical Device Driver Version: {}", vkbPhysicalDevice_.properties.driverVersion);
  log().trace("Selected Physical Device Vulkan API Version: {}.{}.{}",
              VK_API_VERSION_MAJOR(vkbPhysicalDevice_.properties.apiVersion),
              VK_API_VERSION_MINOR(vkbPhysicalDevice_.properties.apiVersion),
              VK_API_VERSION_PATCH(vkbPhysicalDevice_.properties.apiVersion));

  // Create logical device
  vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice_);
  auto vkbDeviceResult = deviceBuilder.build();
  if (!vkbDeviceResult)
    log().fatal("Failed to create Vulkan Logical Device: {}", vkbDeviceResult.error().message());
  vkbDevice_ = vkbDeviceResult.value();
  // Load Vulkan device-level function pointers. Observed Vulkan device
  // functions are loaded without explicit call to this function. But it won't
  // hurt to call it.
  volkLoadDevice(vkbDevice_.device);

  graphicsQueueFamilyIndex_ = vkbDevice_.get_queue_index(vkb::QueueType::graphics).value();
  presentQueueFamilyIndex_ =
      vkbDevice_.get_queue_index(vkb::QueueType::present).value(); // Often same as graphics
  log().trace("graphicsQueueFamilyIndex: {}, presentQueueFamilyIndex: {}", graphicsQueueFamilyIndex_,
              presentQueueFamilyIndex_);
  graphicsQueue_ = vkbDevice_.get_queue(vkb::QueueType::graphics).value();
  presentQueue_ = vkbDevice_.get_queue(vkb::QueueType::present).value();

  // vulkanApiVersion >= VK_API_VERSION_1_4 but required Vulkan version is disabled by preprocessor macros.
  const AllocatorCreateInfo allocatorCreateInfo{
      .vulkanApiVersion = VK_MAKE_VERSION(1, 3, 0),
  };
  allocator_ = Allocator(getInstance(), getPhysicalDevice(), getDevice(), allocatorCreateInfo);
}

VkBool32 VKAPI_PTR debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                 VkDebugUtilsMessageTypeFlagsEXT messageType,
                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                 [[maybe_unused]] void* pUserData) {
  constexpr i32 ignoredMessageIds[] = {
      0x675dc32e, // just warns about VK_EXT_debug_utils is intended to be
                  // used in debugging only.
      0x24b5c69f, // GPU validation:
      0x0,        // Layer name GalaxyOverlayVkLayer does not conform to naming standard (Policy #LLP_LAYER_3)
      -0x257d9f46, // enabling a deprecated extension that has been promoted to main Vulkan. triggered
                   // because of VK_KHR_maintenance6 for vkCmdPushConstants2KHR [issue #7]
      0x86974c1,   // perf warning that says don't use VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                   //  reset entire pool instead.
  };
  for (const auto& msgId : ignoredMessageIds) {
    if (pCallbackData->messageIdNumber == msgId)
      return VK_FALSE; // Skip this message
  }

  const char* type = vkb::to_string_message_type(messageType);
  const char* message = pCallbackData->pMessage;
  // TODO(vug): these labels are probably already included in the debug message. If they appear duplicated
  //            stop adding them to the printed log explicitly.
  std::string queueLabels;
  if (pCallbackData->queueLabelCount > 0) {
    queueLabels = " Queue Labels:";
    for (u32 i = 0; i < pCallbackData->queueLabelCount; ++i)
      queueLabels += std::format(" {}", pCallbackData->pQueueLabels[i].pLabelName);
  }
  std::string commandBufferLabels;
  if (pCallbackData->cmdBufLabelCount > 0) {
    queueLabels = " Command Buffer Labels:";
    for (u32 i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
      queueLabels += std::format(" {}", pCallbackData->pCmdBufLabels[i].pLabelName);
  }

  const std::string debugMsg =
      std::format("VULKAN [{}]: {}{}{}{}", type, message, queueLabels, commandBufferLabels);

  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    log().trace(debugMsg);
    // can return VK_FALSE here to ignore verbose messages
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    log().info(debugMsg);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    log().warn(debugMsg);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    log().error(debugMsg);
    break;
  default:
    log().critical("Unknown Vulkan message severity: {}. message: {}", static_cast<int>(messageSeverity),
                   debugMsg);
  }
  if (kDebugBreakAtValidationErrors)
    std::abort();
  return VK_FALSE;
  // Return true for validation to skip passing down the call to the driver
  // and return VK_ERROR_VALIDATION_FAILED_EXT from VK function calls
  // return VK_TRUE;
};

} // namespace aur