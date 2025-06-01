#pragma once

#include <vk-bootstrap/VkBootstrap.h>

#include <vector>

struct GLFWwindow;

namespace aur {

class VulkanContext;

class Swapchain {
 public:
  Swapchain(const VulkanContext& context, GLFWwindow* window);
  ~Swapchain();

  Swapchain(const Swapchain&) = delete;
  Swapchain& operator=(const Swapchain&) = delete;
  Swapchain(Swapchain&&) = delete;
  Swapchain& operator=(Swapchain&&) = delete;

  void recreate(const VulkanContext& context, GLFWwindow* window);

  const VkSwapchainKHR& getSwapchain() const { return vkbSwapchain_.swapchain; }
  VkFormat getImageFormat() const { return vkbSwapchain_.image_format; }
  VkExtent2D getImageExtent() const { return vkbSwapchain_.extent; }
  const std::vector<VkImageView>& getImageViews() const {
    return imageViews_;
  }
  const std::vector<VkImage>& getImages() const {
    return images_;
  }
  uint32_t getImageCount() const { return vkbSwapchain_.image_count; }

 private:
  void create(const VulkanContext& context, GLFWwindow* window,
              VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
  void destroy();

  vkb::Swapchain vkbSwapchain_;
  GLFWwindow* window_{};
  std::vector<VkImageView> imageViews_;
  std::vector<VkImage> images_;
};

}  // namespace aur