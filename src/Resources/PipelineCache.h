#pragma once

#include "../VulkanWrappers.h"
#include "VulkanResource.h"
#include <vector>

namespace aur {

struct PipelineCacheCreateInfo {
  const std::vector<std::byte>* initialData{};
};

class PipelineCache
    : public VulkanResource<PipelineCache, VkPipelineCache, PipelineCacheCreateInfo, VkDevice> {
public:
  PipelineCache() = default;
  PipelineCache(VkDevice device, const PipelineCacheCreateInfo& createInfo);
  ~PipelineCache();

  PipelineCache(const PipelineCache&) = delete;
  PipelineCache& operator=(const PipelineCache&) = delete;
  PipelineCache(PipelineCache&& other) noexcept = default;
  PipelineCache& operator=(PipelineCache&& other) noexcept = default;

private:
  friend class VulkanResource<PipelineCache, VkPipelineCache, PipelineCacheCreateInfo, VkDevice>;

  static VkPipelineCache createImpl(PipelineCache* self, const PipelineCacheCreateInfo& createInfo,
                                    const std::tuple<VkDevice>& context);
  void destroyImpl() const;
};

} // namespace aur