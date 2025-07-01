#pragma once

#include <filesystem>

#include "Resources/PipelineLayout.h"
#include "VulkanWrappers.h"
#include "asset/Handle.h"

namespace aur {

class Renderer;
class AssetManager;
namespace asset {
struct Shader;
}

struct PipelineCreateInfo {
  Handle<asset::Shader> vert;
  Handle<asset::Shader> frag;
  CullMode cullMode{CullMode::Back};
};

class Pipeline {
public:
  Pipeline() = default;
  Pipeline(AssetManager& assetManager, const Renderer& renderer, const PipelineCreateInfo& createInfo);
  ~Pipeline();

  PipelineLayout pipelineLayout;
  VkPipeline handle{VK_NULL_HANDLE};

private:
  const Renderer& renderer_;
};

} // namespace aur