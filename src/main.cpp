#include <iostream>

// Includes from dependencies
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#include <vk-bootstrap/VkBootstrap.h> // Main header for vk-bootstrap

int main() {
    std::cout << "MyApplication starting..." << std::endl;

    // Example: Initialize Volk to check if it's linked and found.
    // This requires a Vulkan loader (SDK) to be available at runtime.
    VkResult volk_init_result = volkInitialize();
    if (volk_init_result == VK_SUCCESS) {
        std::cout << "Volk initialized successfully." << std::endl;
        // You can now use volkGetVersion() or other Vulkan functions via Volk
        // For example:
        // uint32_t instance_version = 0;
        // if (vkEnumerateInstanceVersion) { // Check if the function pointer is loaded
        //     vkEnumerateInstanceVersion(&instance_version);
        //     std::cout << "Vulkan Instance Version (via Volk): "
        //               << VK_API_VERSION_MAJOR(instance_version) << "."
        //               << VK_API_VERSION_MINOR(instance_version) << "."
        //               << VK_API_VERSION_PATCH(instance_version) << std::endl;
        // }
    } else {
        std::cerr << "Failed to initialize Volk! Error: " << volk_init_result << std::endl;
        // Depending on your needs, this might not be a fatal error if you only
        // use Volk to load a subset of functions or handle its absence.
    }

    // Example: Use a type/function from vk-bootstrap to check linkage.
    auto system_info_ret = vkb::SystemInfo::get_system_info(); // This uses Vulkan functions
    
    if (volk_init_result == VK_SUCCESS && vkGetInstanceProcAddr != nullptr) { // Ensure Vulkan is somewhat usable
        if (system_info_ret) {
            vkb::SystemInfo system_info = system_info_ret.value();
            std::cout << "vk-bootstrap SystemInfo obtained." << std::endl;
            // if (system_info.is_debug_utils_available()) {
            //     std::cout << "Vulkan Debug Utils are available according to vk-bootstrap." << std::endl;
            // } else {
            //     std::cout << "Vulkan Debug Utils are NOT available according to vk-bootstrap." << std::endl;
            // }
        } else {
            std::cerr << "Failed to get vk-bootstrap SystemInfo: " << system_info_ret.error().message() << std::endl;
        }
    } else {
        std::cout << "Skipping vk-bootstrap SystemInfo check as Vulkan (via Volk) is not fully initialized." << std::endl;
    }


    std::cout << "MyApplication finished." << std::endl;
    return 0;
}

