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

  // Compare members in a fixed order.
  auto identifier() const { return std::tie(vert.id, frag.id, cullMode); }
  bool operator<(const PipelineCreateInfo& other) const { return identifier() < other.identifier(); }
};

class Pipeline {
public:
  Pipeline() = default;
  Pipeline(const Renderer& renderer, const PipelineCreateInfo& createInfo);
  ~Pipeline();

  PipelineLayout pipelineLayout;
  VkPipeline handle{VK_NULL_HANDLE};

private:
  const Renderer& renderer_;
};

} // namespace aur