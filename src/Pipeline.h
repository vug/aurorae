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

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;
  Pipeline(Pipeline&& other) noexcept;
  Pipeline& operator=(Pipeline&& other) noexcept;

  const VkPipeline& getHandle() const { return handle_; }
  const PipelineCreateInfo& getCreateInfo() const { return createInfo_; }
  [[nodiscard]] inline bool isValid() const { return handle_ != VK_NULL_HANDLE; }

  const PipelineLayout& getPipelineLayout() const { return pipelineLayout_; }

private:
  void invalidate();
  void destroy();

  PipelineCreateInfo createInfo_;
  const Renderer* renderer_;
  PipelineLayout pipelineLayout_;
  VkPipeline handle_{VK_NULL_HANDLE};
};

} // namespace aur