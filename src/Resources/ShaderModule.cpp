#include "ShaderModule.h"

#include <volk/volk.h>

#include "../FileIO.h"
#include "../Logger.h"

namespace aur {

ShaderModule::ShaderModule(VkDevice device, const ShaderModuleCreateInfo& shaderCreateInfo)
    : VulkanResource{shaderCreateInfo, device} {}

ShaderModule::~ShaderModule() {
  destroy();
}

// Static method to create the handle, called by the base class.
VkShaderModule ShaderModule::createImpl(const ShaderModuleCreateInfo& createInfo,
                                        const std::tuple<VkDevice>& context) {
  const VkShaderModuleCreateInfo vkCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = createInfo.codeBlob->size(),
      .pCode = reinterpret_cast<const uint32_t*>(createInfo.codeBlob->data()),
  };
  VkShaderModule hnd = VK_NULL_HANDLE;
  if (vkCreateShaderModule(std::get<0>(context), &vkCreateInfo, nullptr, &hnd) != VK_SUCCESS)
    log().fatal("Failed to create shader module!");

  return hnd;
}

void ShaderModule::destroyImpl() {
  vkDestroyShaderModule(std::get<0>(context_), handle_, nullptr);
}

} // namespace aur