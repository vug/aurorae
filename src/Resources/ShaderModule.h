#pragma once

#include <filesystem>

#include "../VulkanWrappers.h"
#include "VulkanResource.h"

namespace aur {
struct ShaderModuleCreateInfo {
  const std::vector<u32>* spirv{};
};

class ShaderModule : public VulkanResource<ShaderModule, VkShaderModule, ShaderModuleCreateInfo, VkDevice> {
public:
  ShaderModule() = default;
  ShaderModule(VkDevice device, const ShaderModuleCreateInfo& createInfo);
  ~ShaderModule();

  // Deleted copy functions are not technically needed, but silence clang-tidy and clarify intent:
  ShaderModule(const ShaderModule&) = delete;
  ShaderModule& operator=(const ShaderModule&) = delete;
  ShaderModule(ShaderModule&& other) noexcept = default;
  ShaderModule& operator=(ShaderModule&& other) noexcept = default;

private:
  // Grant access to the base class to call implementation methods.
  friend class VulkanResource<ShaderModule, VkShaderModule, ShaderModuleCreateInfo, VkDevice>;

  // The static creator function called by the base class constructor.
  static VkShaderModule createImpl(ShaderModule* self, const ShaderModuleCreateInfo& createInfo,
                                   const std::tuple<VkDevice>& context);
  // The destroyer function called by the base class.
  void destroyImpl();
};

} // namespace aur