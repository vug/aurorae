#pragma once

#include <filesystem>

#include "Resources/PipelineLayout.h"
#include "VulkanWrappers.h"

FORWARD_DEFINE_VK_HANDLE(VkPipelineLayout)

namespace aur {
// TODO(vug): temporarily defined to group certain resources together. Proto-material
struct Pipeline {
  std::filesystem::path vertexPath;
  std::filesystem::path fragmentPath;
  VkPipeline pipeline{VK_NULL_HANDLE};
  PipelineLayout pipelineLayout;
};

} // namespace aur