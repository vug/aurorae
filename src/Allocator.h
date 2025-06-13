#pragma once

#include "Utils.h"

FORWARD_DEFINE_VK_HANDLE(VmaAllocator)

namespace aur {

class VulkanContext;

class Allocator {
public:
  explicit Allocator(const VulkanContext& context);
  ~Allocator();

  // move-only
  Allocator(const Allocator&) = delete;
  Allocator& operator=(const Allocator&) = delete;
  Allocator(Allocator&& other) noexcept;
  Allocator& operator=(Allocator&& other) noexcept;

  // Getter for the raw VMA handle
  [[nodiscard]] VmaAllocator getHandle() const { return handle_; }

private:
  VmaAllocator handle_{VK_NULL_HANDLE};
};

} // namespace aur