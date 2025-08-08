#pragma once

#include <filesystem>

#include "Handle.h"
#include "Resources/PipelineLayout.h"
#include "VulkanWrappers.h"

// boost::hash_combine
inline void hashCombine(size_t& seed, size_t hash) {
  seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace aur {

class Renderer;
class AssetManager;
namespace render {
class GraphicsProgram;
}

struct PipelineCreateInfo {
  // Increment version after each change to the schema or processing logic
  static constexpr u32 schemaVersion{1};
  u32 version{schemaVersion};

  Handle<render::GraphicsProgram> graphicsProgram;
  CullMode cullMode{CullMode::Back};

  // Compare members in a fixed order.
  [[nodiscard]] auto identifier() const { return std::tie(graphicsProgram.id, cullMode); }
  bool operator<(const PipelineCreateInfo& other) const { return identifier() < other.identifier(); }
  bool operator==(const PipelineCreateInfo& other) const { return identifier() == other.identifier(); }
};

class Pipeline {
public:
  Pipeline() = default;
  Pipeline(Renderer& renderer, const PipelineCreateInfo& createInfo);
  ~Pipeline();

  Pipeline(const Pipeline&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;
  Pipeline(Pipeline&& other) noexcept;
  Pipeline& operator=(Pipeline&& other) noexcept;

  [[nodiscard]] const PipelineCreateInfo& getCreateInfo() const { return createInfo_; }
  [[nodiscard]] const VkPipeline& getHandle() const { return handle_; }
  [[nodiscard]] bool isValid() const { return handle_ != VK_NULL_HANDLE; }

  [[nodiscard]] const PipelineLayout& getPipelineLayout() const { return pipelineLayout_; }

private:
  void invalidate();
  void destroy();

  PipelineCreateInfo createInfo_;
  const Renderer* renderer_{};
  PipelineLayout pipelineLayout_;
  VkPipeline handle_{VK_NULL_HANDLE};
};

} // namespace aur

template <>
struct std::hash<aur::PipelineCreateInfo> {
  size_t operator()(const aur::PipelineCreateInfo& createInfo) const noexcept {
    const size_t shaderHash = std::hash<aur::u32>()(createInfo.graphicsProgram.id);
    const size_t cullModeHash =
        std::hash<std::underlying_type_t<aur::CullMode>>()(std::to_underlying(createInfo.cullMode));
    // left shift helps with distributing bits
    const size_t hash = shaderHash ^ (cullModeHash << 1);
    return hash;
  }
}; // namespace std