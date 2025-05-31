#pragma once

#include <vk-bootstrap/VkBootstrap.h>

#include <string_view>

struct GLFWwindow;

namespace aur {

class VulkanContext {
public:
    VulkanContext(GLFWwindow* window, std::string_view appName);
    ~VulkanContext();

    // Delete copy/move constructors to prevent accidental duplication
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    // Public getters for raw Vulkan handles
    inline VkInstance getInstance() const { return vkbInstance_.instance; }
    VkDevice getDevice() const { return vkbDevice_.device; }
    VkPhysicalDevice getPhysicalDevice() const { return vkbPhysicalDevice_.physical_device; }
    VkSurfaceKHR getSurface() const { return surface_; }

    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    uint32_t getGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
    uint32_t getPresentQueueFamilyIndex() const { return m_presentQueueFamilyIndex; }

    const vkb::Instance& getVkbInstance() const { return vkbInstance_; }
    const vkb::PhysicalDevice& getVkbPhysicalDevice() const { return vkbPhysicalDevice_; }
    const vkb::Device& getVkbDevice() const { return vkbDevice_; }

private:
    vkb::Instance vkbInstance_;
    vkb::PhysicalDevice vkbPhysicalDevice_;
    vkb::Device vkbDevice_;

    VkSurfaceKHR surface_{VK_NULL_HANDLE};
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamilyIndex = 0;
    uint32_t m_presentQueueFamilyIndex = 0;

    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
};

} // namespace aur