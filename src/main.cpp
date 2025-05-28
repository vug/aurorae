#include <iostream>
#include <print>

// Includes from dependencies
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#include <vk-bootstrap/VkBootstrap.h>  // Main header for vk-bootstrap
#include <spdlog/spdlog.h>

int main() {
  spdlog::info("Hi!");

  // Make sure Vulkan SDK is installed
  VkResult volkInitResult = volkInitialize();
  if (volkInitResult != VK_SUCCESS) {
    std::println(std::cerr, "[ERROR] Volk initialization failed! Error {}",
                 static_cast<int>(volkInitResult));
    return -1;  // Exit early if Vulkan is not initialized
  }

  uint32_t instance_version = 0;
  if (vkEnumerateInstanceVersion) {  // Check if the function pointer is
                                     // loaded
    vkEnumerateInstanceVersion(&instance_version);
    std::println("Vulkan Instance Version (via Volk): {}.{}.{}",
                 VK_API_VERSION_MAJOR(instance_version),
                 VK_API_VERSION_MINOR(instance_version),
                 VK_API_VERSION_PATCH(instance_version));
  }

  auto systemInfoRes = vkb::SystemInfo::get_system_info();
  if (vkGetInstanceProcAddr != nullptr) {
    if (systemInfoRes) {
      vkb::SystemInfo systemInfo = systemInfoRes.value();
      std::println(
          "vk-bootstrap SystemInfo obtained. Validation Layer available? {}",
          systemInfo.validation_layers_available);
    } else {
      std::println(std::cerr,
                   "[ERROR] Failed to get vk-bootstrap SystemInfo: {}",
                   systemInfoRes.error().message());
    }
  } else {
    std::println(
        "Skipping vk-bootstrap SystemInfo check as Vulkan (via Volk) is not "
        "fully initialized.");
  }

  spdlog::info("Bye!");
  return 0;
}
