#include "Swapchain.h"

#include <glfw/glfw3.h>

#include "utils.h"
#include "VulkanContext.h"

namespace aur {

// TODO(vug): don't give VulkanContext, just the device
Swapchain::Swapchain(const VulkanContext& context, GLFWwindow* window)
    : window_(window) {
  create(context, window);
}

Swapchain::~Swapchain() { destroy(); }

// TODO(vug): GLFWwindow is not needed just take size
void Swapchain::create(const VulkanContext& context, GLFWwindow* window,
                       VkSwapchainKHR oldSwapchain) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  vkb::SwapchainBuilder swapchainBuilder(context.getVkbDevice());
  auto vkbSwapchainResult =
      swapchainBuilder
        .set_old_swapchain(oldSwapchain)
        .set_desired_extent((uint32_t)width, (uint32_t)height)
        .set_desired_format(VkSurfaceFormatKHR{
          .format = VK_FORMAT_B8G8R8A8_SRGB,
          .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        })
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .build();
  if (!vkbSwapchainResult)
    fatal("Failed to create Vulkan Swapchain: {}", vkbSwapchainResult.error().message());        

  vkbSwapchain_ = vkbSwapchainResult.value();

  auto imageViewsResult = vkbSwapchain_.get_image_views();
  if (!imageViewsResult)
    fatal("Failed to get swapchain image views: {}", imageViewsResult.error().message());
  imageViews_ = imageViewsResult.value();
}

void Swapchain::destroy() {
  if (vkbSwapchain_.device != VK_NULL_HANDLE)
    vkbSwapchain_.destroy_image_views(imageViews_);
  imageViews_.clear();
  vkb::destroy_swapchain(vkbSwapchain_); // Destroys VkSwapchainKHR, VkImageViews, etc.
}

void Swapchain::recreate(const VulkanContext& context, GLFWwindow* window) {
  // TODO(vug): move recreate logic to main app -> now we can decouple GLFW
  // Handle minimization: pause until window is restored
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(context.getDevice());
  VkSwapchainKHR oldSwapchain = vkbSwapchain_.swapchain;
  destroy();
  create(context, window, oldSwapchain);
}

}  // namespace aur