#pragma once

#include <tuple>
#include <utility>

#include "../VulkanWrappers.h"

namespace aur {

/**
 * A CRTP base class for managing move-only Vulkan resources.
 *
 * @tparam Derived The concrete resource class (e.g., ShaderModule).
 * @tparam HandleType The Vulkan handle type (e.g., VkShaderModule).
 * @tparam CreateInfoType The creation info struct type.
 * @tparam Contexts A variadic list of context types required for creation/destruction
 *                  (e.g., VkDevice, VmaAllocator).
 */
template <typename Derived, typename HandleType, typename CreateInfoType, typename... Contexts>
class VulkanResource {
public:
  VulkanResource() = default;
  ~VulkanResource() = default; // The derived class destructor must call destroy().

  // The class is move-only.
  VulkanResource(const VulkanResource&) = delete;
  VulkanResource& operator=(const VulkanResource&) = delete;

  VulkanResource(VulkanResource&& other) noexcept
      : createInfo_{std::exchange(other.createInfo_, {})}
      , context_{std::exchange(other.context_, {})}
      , handle_{std::exchange(other.handle_, VK_NULL_HANDLE)} {}

  VulkanResource& operator=(VulkanResource&& other) noexcept {
    if (this != &other) {
      // Destroy the current resource before overwriting its state,
      // as the destroy implementation may need the old context.
      static_cast<Derived*>(this)->destroy();

      createInfo_ = std::exchange(other.createInfo_, {});
      context_ = std::exchange(other.context_, {});
      handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
    }
    return *this;
  }

  // Common getters
  [[nodiscard]] const HandleType& getHandle() const { return handle_; }
  [[nodiscard]] const CreateInfoType& getCreateInfo() const { return createInfo_; }
  [[nodiscard]] bool isValid() const { return handle_ != VK_NULL_HANDLE; }

  /**
   * Destroys the underlying Vulkan resource if it's valid.
   */
  void destroy() {
    if (isValid()) {
      static_cast<Derived*>(this)->destroyImpl();
      invalidate();
    }
  }

protected:
  // Constructor for derived classes to initialize all base members.
  VulkanResource(const CreateInfoType& createInfo, Contexts... contexts)
      : createInfo_{createInfo}
      , context_{std::make_tuple(contexts...)}
      , handle_{Derived::createImpl(createInfo_, context_)} {}

  // Let derived classes access their state.
  // The order is important for the initializer list!
  CreateInfoType createInfo_{};
  std::tuple<Contexts...> context_{};
  HandleType handle_{VK_NULL_HANDLE};

private:
  void invalidate() { handle_ = VK_NULL_HANDLE; }
};

} // namespace aur