#pragma once

#include <filesystem>

#include "../VulkanWrappers.h"

namespace aur {
class BinaryBlob;

struct ShaderModuleCreateInfo {
  const BinaryBlob* codeBlob;
};

class ShaderModule {
public:
  ShaderModule() = default;
  ShaderModule(VkDevice device, const ShaderModuleCreateInfo& createInfo);
  ~ShaderModule();

  ShaderModule(const ShaderModule&) = delete;
  ShaderModule& operator=(const ShaderModule&) = delete;
  ShaderModule(ShaderModule&& other) noexcept;
  ShaderModule& operator=(ShaderModule&& other) noexcept;

  [[nodiscard]] const ShaderModuleCreateInfo& getCreateInfo() const { return createInfo; }
  [[nodiscard]] const VkShaderModule& getHandle() const { return handle; }
  [[nodiscard]] inline bool isValid() const { return handle != VK_NULL_HANDLE; }

private:
  void invalidate();
  void destroy();

  ShaderModuleCreateInfo createInfo;
  VkShaderModule handle{VK_NULL_HANDLE};
  VkDevice device_{VK_NULL_HANDLE};
};

} // namespace aur