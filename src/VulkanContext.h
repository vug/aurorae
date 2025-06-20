#pragma once

#define VK_NO_PROTOTYPES
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
  [[nodiscard]] inline const VkInstance& getInstance() const { return vkbInstance_.instance; }
  [[nodiscard]] inline const VkDevice& getDevice() const { return vkbDevice_.device; }
  [[nodiscard]] inline const VkPhysicalDevice& getPhysicalDevice() const {
    return vkbPhysicalDevice_.physical_device;
  }
  [[nodiscard]] inline const VkSurfaceKHR& getSurface() const { return surface_; }

  [[nodiscard]] VkQueue getGraphicsQueue() const { return graphicsQueue_; }
  [[nodiscard]] VkQueue getPresentQueue() const { return presentQueue_; }
  [[nodiscard]] u32 getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex_; }
  [[nodiscard]] u32 getPresentQueueFamilyIndex() const { return presentQueueFamilyIndex_; }

  [[nodiscard]] const vkb::Instance& getVkbInstance() const { return vkbInstance_; }
  [[nodiscard]] const vkb::PhysicalDevice& getVkbPhysicalDevice() const { return vkbPhysicalDevice_; }
  [[nodiscard]] const vkb::Device& getVkbDevice() const { return vkbDevice_; }

private:
  vkb::Instance vkbInstance_;
  vkb::PhysicalDevice vkbPhysicalDevice_;
  vkb::Device vkbDevice_;

  VkSurfaceKHR surface_{VK_NULL_HANDLE};
  VkQueue graphicsQueue_{VK_NULL_HANDLE};
  VkQueue presentQueue_{VK_NULL_HANDLE};
  u32 graphicsQueueFamilyIndex_{};
  u32 presentQueueFamilyIndex_{};
};

} // namespace aur