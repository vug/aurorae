#pragma once

#include <filesystem>

#include "../VulkanWrappers.h"

namespace aur {
class BinaryBlob;

struct ShaderModuleCreateInfo {
  const std::vector<std::byte>* codeBlob{};
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

  [[nodiscard]] const ShaderModuleCreateInfo& getCreateInfo() const { return createInfo_; }
  [[nodiscard]] const VkShaderModule& getHandle() const { return handle_; }
  [[nodiscard]] inline bool isValid() const { return handle_ != VK_NULL_HANDLE; }

private:
  void invalidate();
  void destroy();

  VkDevice device_{VK_NULL_HANDLE};
  ShaderModuleCreateInfo createInfo_{};
  VkShaderModule handle_{VK_NULL_HANDLE};
};

} // namespace aur