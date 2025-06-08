#pragma once

#include <vk-bootstrap/VkBootstrap.h>

#include <vector>

#include "Utils.h"

namespace aur {

class Swapchain {
 public:  // vkb::Device is defined in vk-bootstrap/VkBootstrap.h
  Swapchain(const vkb::Device& vkb_device, u32 width, u32 height);
  ~Swapchain();

  Swapchain(const Swapchain&) = delete;
  Swapchain& operator=(const Swapchain&) = delete;
  Swapchain(Swapchain&&) = delete;
  Swapchain& operator=(Swapchain&&) = delete;
  void recreate(const vkb::Device& vkb_device, u32 width, u32 height);

  const VkSwapchainKHR& getSwapchain() const { return vkbSwapchain_.swapchain; }
  VkFormat getImageFormat() const { return vkbSwapchain_.image_format; }
  VkExtent2D getImageExtent() const { return vkbSwapchain_.extent; }
  const std::vector<VkImageView>& getImageViews() const { return imageViews_; }
  const std::vector<VkImage>& getImages() const { return images_; }
  u32 getImageCount() const { return vkbSwapchain_.image_count; }

 private:
  void create(const vkb::Device& vkb_device, u32 width, u32 height,
              VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
  void destroy();

  vkb::Swapchain vkbSwapchain_;
  std::vector<VkImageView> imageViews_;
  std::vector<VkImage> images_;
};

}  // namespace aur