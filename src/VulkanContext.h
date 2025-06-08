#pragma once

#include <vk-bootstrap/VkBootstrap.h>

struct GLFWwindow;

namespace aur {

class VulkanContext {
 public:
  VulkanContext(GLFWwindow* window, const char* appName);
  ~VulkanContext();

  // Delete copy/move constructors to prevent accidental duplication
  VulkanContext(const VulkanContext&) = delete;
  VulkanContext& operator=(const VulkanContext&) = delete;
  VulkanContext(VulkanContext&&) = delete;
  VulkanContext& operator=(VulkanContext&&) = delete;

  // Public getters for raw Vulkan handles
  inline VkInstance getInstance() const { return vkbInstance_.instance; }
  VkDevice getDevice() const { return vkbDevice_.device; }
  VkPhysicalDevice getPhysicalDevice() const {
    return vkbPhysicalDevice_.physical_device;
  }
  VkSurfaceKHR getSurface() const { return surface_; }

  VkQueue getGraphicsQueue() const { return graphicsQueue_; }
  VkQueue getPresentQueue() const { return presentQueue_; }
  uint32_t getGraphicsQueueFamilyIndex() const {
    return graphicsQueueFamilyIndex_;
  }
  uint32_t getPresentQueueFamilyIndex() const {
    return presentQueueFamilyIndex_;
  }

  const vkb::Instance& getVkbInstance() const { return vkbInstance_; }
  const vkb::PhysicalDevice& getVkbPhysicalDevice() const {
    return vkbPhysicalDevice_;
  }
  const vkb::Device& getVkbDevice() const { return vkbDevice_; }

 private:
  vkb::Instance vkbInstance_;
  vkb::PhysicalDevice vkbPhysicalDevice_;
  vkb::Device vkbDevice_;

  VkSurfaceKHR surface_{VK_NULL_HANDLE};
  VkQueue graphicsQueue_{VK_NULL_HANDLE};
  VkQueue presentQueue_{VK_NULL_HANDLE};
  uint32_t graphicsQueueFamilyIndex_{};
  uint32_t presentQueueFamilyIndex_{};
};

}  // namespace aur