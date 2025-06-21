#pragma once

#include "Resources/PipelineLayout.h"
#include "Utils.h"

FORWARD_DEFINE_VK_HANDLE(VkPipeline)
FORWARD_DEFINE_VK_HANDLE(VkPipelineLayout)

namespace aur {
// TODO(vug): temporarily defined to group certain resources together. Proto-material
struct Pipeline {
  PathBuffer vertexPath;
  PathBuffer fragmentPath;
  VkPipeline pipeline{VK_NULL_HANDLE};
  PipelineLayout pipelineLayout;
};

} // namespace aur