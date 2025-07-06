#include "PipelineCache.h"

#include <volk/volk.h>

#include "../Logger.h"

namespace aur {

PipelineCache::PipelineCache(VkDevice device, const PipelineCacheCreateInfo& createInfo)
    : VulkanResource{createInfo, device} {}

PipelineCache::~PipelineCache() {
  destroy();
}

VkPipelineCache PipelineCache::createImpl([[maybe_unused]] PipelineCache* self,
                                          const PipelineCacheCreateInfo& createInfo,
                                          const std::tuple<VkDevice>& context) {
  const VkPipelineCacheCreateInfo vkCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
      .initialDataSize = createInfo.initialData ? createInfo.initialData->size() : 0,
      .pInitialData = createInfo.initialData ? createInfo.initialData->data() : nullptr,
  };

  VkPipelineCache hnd = VK_NULL_HANDLE;
  VK(vkCreatePipelineCache(std::get<0>(context), &vkCreateInfo, nullptr, &hnd));

  return hnd;
}

void PipelineCache::destroyImpl() const {
  vkDestroyPipelineCache(std::get<0>(context_), handle_, nullptr);
}

} // namespace aur