#include "ShaderModule.h"

#include <volk/volk.h>

#include "../FileIO.h"
#include "../Logger.h"

namespace aur {

ShaderModule::ShaderModule(VkDevice device, const ShaderModuleCreateInfo& shaderCreateInfo)
    : device_{device}
    , createInfo_{shaderCreateInfo}
    , handle_{[this, device]() {
      const VkShaderModuleCreateInfo vkCreateInfo{
          .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
          .codeSize = createInfo_.codeBlob->size(),
          .pCode = reinterpret_cast<const u32*>(createInfo_.codeBlob->data()),
      };
      VkShaderModule hnd = VK_NULL_HANDLE;
      if (vkCreateShaderModule(device, &vkCreateInfo, nullptr, &hnd) != VK_SUCCESS)
        log().fatal("Failed to create shader module!");

      return hnd;
    }()} {}

ShaderModule::~ShaderModule() {
  destroy();
}

ShaderModule::ShaderModule(ShaderModule&& other) noexcept
    : device_{std::exchange(other.device_, {})}
    , createInfo_{std::exchange(other.createInfo_, {})}
    , handle_{std::exchange(other.handle_, {})} {
  other.invalidate();
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept {
  if (this == &other)
    return *this;

  destroy();

  device_ = std::exchange(other.device_, {});
  createInfo_ = std::exchange(other.createInfo_, {});
  handle_ = std::exchange(other.handle_, {});

  return *this;
}

void ShaderModule::invalidate() {
  handle_ = VK_NULL_HANDLE;
}

void ShaderModule::destroy() {
  if (!isValid())
    return;

  vkDestroyShaderModule(device_, handle_, nullptr);
  invalidate();
}

} // namespace aur