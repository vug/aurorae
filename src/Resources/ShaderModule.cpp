#include "ShaderModule.h"

#include <volk/volk.h>

#include "../FileIO.h"
#include "../Logger.h"

namespace aur {

ShaderModule::ShaderModule(VkDevice device, const ShaderModuleCreateInfo& shaderCreateInfo)
    : createInfo(shaderCreateInfo)
    , handle([this, device]() {
      BinaryBlob codeBlob = readBinaryFile(createInfo.filePath.string());

      VkShaderModuleCreateInfo vkCreateInfo{
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .codeSize = codeBlob.size(),
          .pCode = reinterpret_cast<const u32*>(codeBlob.data()),
      };
      VkShaderModule hnd = VK_NULL_HANDLE;
      if (vkCreateShaderModule(device, &vkCreateInfo, nullptr, &hnd) != VK_SUCCESS)
        log().fatal("Failed to create shader module!");

      return hnd;
    }())
    , device_(device) {}

ShaderModule::~ShaderModule() {
  destroy();
}

ShaderModule::ShaderModule(ShaderModule&& other) noexcept
    : createInfo(std::move(const_cast<ShaderModuleCreateInfo&>(other.createInfo)))
    , handle(other.handle)
    , device_(other.device_) {
  other.invalidate();
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept {
  if (this != &other) {
    destroy();
    const_cast<ShaderModuleCreateInfo&>(createInfo) =
        std::move(const_cast<ShaderModuleCreateInfo&>(other.createInfo));
    const_cast<VkShaderModule&>(handle) = other.handle;
    device_ = other.device_;
    other.invalidate();
  }
  return *this;
}

void ShaderModule::invalidate() {
  const_cast<VkShaderModule&>(handle) = VK_NULL_HANDLE;
}

void ShaderModule::destroy() {
  if (isValid() && device_ != VK_NULL_HANDLE)
    vkDestroyShaderModule(device_, handle, nullptr);
  invalidate();
}

} // namespace aur