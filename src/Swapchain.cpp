// volk needs to be included before Swapchain.
// clang-format off
#include <volk/volk.h>
#include "Swapchain.h"
// clang-format on

#include "Logger.h"

namespace aur {

Swapchain::Swapchain(const vkb::Device& vkb_device, u32 width, u32 height) {
  create(vkb_device, width, height);
}

Swapchain::~Swapchain() {
  destroy();
}

void Swapchain::create(const vkb::Device& vkb_device, u32 width, u32 height, VkSwapchainKHR oldSwapchain) {
  vkb::SwapchainBuilder swapchainBuilder(vkb_device);
  auto vkbSwapchainResult =
      swapchainBuilder.set_old_swapchain(oldSwapchain)
          .set_desired_extent(width, height)
          .set_desired_format(VkSurfaceFormatKHR{.format = VK_FORMAT_B8G8R8A8_SRGB,
                                                 .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
          .set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR) // No v-sync, may tear but faster. Use this
                                                                   // when swapchain image count is 2
          // .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // V-sync on
          // .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR) // triple buffering, smooth&fast, complex
          .set_desired_min_image_count(2)
          .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
          .build();
  if (!vkbSwapchainResult)
    log().fatal("Failed to create Vulkan Swapchain: {}", vkbSwapchainResult.error().message());

  vkbSwapchain_ = vkbSwapchainResult.value();

  auto imageViewsResult = vkbSwapchain_.get_image_views();
  if (!imageViewsResult)
    log().fatal("Failed to get swapchain image views: {}", imageViewsResult.error().message());
  imageViews_ = imageViewsResult.value();

  auto imagesResult = vkbSwapchain_.get_images();
  if (!imagesResult)
    log().fatal("Failed to get swapchain images: {}", imagesResult.error().message());
  images_ = imagesResult.value();

  log().debug("Swapchain created with {} images.", getImageCount());
}

void Swapchain::destroy() {
  if (vkbSwapchain_.device != VK_NULL_HANDLE)
    vkbSwapchain_.destroy_image_views(imageViews_);
  imageViews_.clear();
  vkb::destroy_swapchain(vkbSwapchain_); // Destroys VkSwapchainKHR, VkImageViews, etc.
}

void Swapchain::recreate(const vkb::Device& vkb_device, u32 width, u32 height) {
  // Minimization handling (waiting for non-zero width/height)
  // is expected to be done by the caller
  VK(vkDeviceWaitIdle(vkb_device.device));

  vkb::Swapchain vkbSwapchainToDestroy = vkbSwapchain_; // Shallow copy of the vkb::Swapchain struct
  const std::vector<VkImageView> imageViewsToDestroy = imageViews_;

  // New swapchain created, oldSwapchain (vkbSwapchainToDestroy.swapchain) is
  // retired. Its resources might be internally transitioned or marked for
  // cleanup.
  create(vkb_device, width, height, vkbSwapchainToDestroy.swapchain);

  // Now it's safe to destroy the retired swapchain handle and its views,
  // as the GPU was idle and the new swapchain creation has handled the
  // transition.
  vkbSwapchainToDestroy.destroy_image_views(imageViewsToDestroy);
  vkb::destroy_swapchain(vkbSwapchainToDestroy); // Destroys the old VkSwapchainKHR
}
} // namespace aur