#pragma once

#include <filesystem>

#include "../FileIO.h"
#include "../VulkanWrappers.h"

namespace aur {

struct ShaderModuleCreateInfo {
  std::filesystem::path filePath;
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

  [[nodiscard]] bool isValid() const { return handle != VK_NULL_HANDLE; }

public:
  const ShaderModuleCreateInfo createInfo;
  const VkShaderModule handle{VK_NULL_HANDLE};

private:
  void invalidate();
  void destroy();

  VkDevice device_{VK_NULL_HANDLE};
};

} // namespace aur