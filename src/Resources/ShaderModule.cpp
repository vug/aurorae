#include "ShaderModule.h"

#include <volk/volk.h>

#include "../Logger.h"

namespace aur {

ShaderModule::ShaderModule(VkDevice device, ShaderModuleCreateInfo shaderCreateInfo)
    : createInfo(std::move(shaderCreateInfo))
    , handle([this, device]() {
      VkShaderModuleCreateInfo vkCreateInfo{
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .codeSize = createInfo.code.size(),
          .pCode = reinterpret_cast<const u32*>(createInfo.code.data()),
      };

      VkShaderModule hnd = VK_NULL_HANDLE;
      if (vkCreateShaderModule(device, &vkCreateInfo, nullptr, &hnd) != VK_SUCCESS)
        log().fatal("Failed to create shader module!");

      // *const_cast<VkShaderModule*>(&handle) = newHandle;
      return hnd;
    }())
    , device_(device) {}

ShaderModule::~ShaderModule() {
  destroy();
}

ShaderModule::ShaderModule(ShaderModule&& other) noexcept
    // TODO(vug): apparently moving a const object is Undefined Behavior. Consider switching to `const
    //            CreateInfo& getCreateInfo()` style.
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