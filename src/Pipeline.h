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
} // namespace aur

namespace aur {
struct PipelineRasterizationStateCreateInfo {
  // Can be set dynamically https://registry.khronos.org/vulkan/specs/latest/man/html/VkFrontFace.html
  PolygonMode polygonMode{PolygonMode::Fill};
  CullMode cullMode{CullMode::Back};
  // Can be set dynamically
  // https://registry.khronos.org/vulkan/specs/latest/man/html/vkCmdSetFrontFaceEXT.html
  FrontFace frontFace{FrontFace::CounterClockwise};
  f32 lineWidth{1.0f};

  [[nodiscard]] VkPipelineRasterizationStateCreateInfo toVk() const {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = static_cast<VkPolygonMode>(polygonMode),
        .cullMode = static_cast<VkCullModeFlags>(cullMode),
        .frontFace = static_cast<VkFrontFace>(frontFace),
        .depthBiasEnable = VK_FALSE,
        .lineWidth = lineWidth,
    };
  }
  // Compare members in a fixed order.
  [[nodiscard]] auto identifier() const { return std::tie(cullMode, polygonMode, frontFace, lineWidth); }
  bool operator<(const PipelineRasterizationStateCreateInfo& other) const {
    return identifier() < other.identifier();
  }
  bool operator==(const PipelineRasterizationStateCreateInfo& other) const {
    return identifier() == other.identifier();
  }
};
} // namespace aur
// clang-format off
template <>
struct std::hash<aur::PipelineRasterizationStateCreateInfo> {
  size_t operator()(const aur::PipelineRasterizationStateCreateInfo& info) const noexcept {
    size_t seed = 0;
    hashCombine(seed, std::hash<std::underlying_type_t<aur::CullMode>>()(std::to_underlying(info.cullMode)));
    hashCombine(seed, std::hash<std::underlying_type_t<aur::PolygonMode>>()(std::to_underlying(info.polygonMode)));
    hashCombine(seed, std::hash<std::underlying_type_t<aur::FrontFace>>()(std::to_underlying(info.frontFace)));
    hashCombine(seed, std::hash<aur::f32>()(info.lineWidth));
    return seed;
  }
};
// clang-format on

namespace aur {
struct PipelineDepthStencilStateCreateInfo {
  bool depthTest{true};
  bool depthWrite{true};

  [[nodiscard]] VkPipelineDepthStencilStateCreateInfo toVk() const {
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = depthTest,
        .depthWriteEnable = depthWrite,
        .depthCompareOp = VK_COMPARE_OP_LESS, // Standard depth test
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };
  }
  // Compare members in a fixed order.
  [[nodiscard]] auto identifier() const { return std::tie(depthTest, depthWrite); }
  bool operator<(const PipelineDepthStencilStateCreateInfo& other) const {
    return identifier() < other.identifier();
  }
  bool operator==(const PipelineDepthStencilStateCreateInfo& other) const {
    return identifier() == other.identifier();
  }
};
} // namespace aur
// clang-format off
template <>
struct std::hash<aur::PipelineDepthStencilStateCreateInfo> {
  size_t operator()(const aur::PipelineDepthStencilStateCreateInfo& info) const noexcept {
    size_t seed = 0;
    hashCombine(seed, std::hash<bool>()(info.depthTest));
    hashCombine(seed, std::hash<bool>()(info.depthWrite));
    return seed;
  }
};
// clang-format on

namespace aur {
struct PipelineColorBlendAttachmentState {
  bool enable{false};
  BlendFactor srcColorFactor{BlendFactor::One};
  BlendFactor dstColorFactor{BlendFactor::Zero};
  BlendOp colorOp{BlendOp::Add};
  BlendFactor srcAlphaFactor{BlendFactor::One};
  BlendFactor dstAlphaFactor{BlendFactor::Zero};
  BlendOp alphaOp{BlendOp::Add};
  std::vector<ColorComponent> writeMask{ColorComponent::Red, ColorComponent::Green, ColorComponent::Blue,
                                        ColorComponent::Alpha};

  [[nodiscard]] VkPipelineColorBlendAttachmentState toVk() const {
    return {
        .blendEnable = enable,
        .srcColorBlendFactor = static_cast<VkBlendFactor>(srcColorFactor),
        .dstColorBlendFactor = static_cast<VkBlendFactor>(dstColorFactor),
        .colorBlendOp = static_cast<VkBlendOp>(colorOp),
        .srcAlphaBlendFactor = static_cast<VkBlendFactor>(srcAlphaFactor),
        .dstAlphaBlendFactor = static_cast<VkBlendFactor>(dstAlphaFactor),
        .alphaBlendOp = static_cast<VkBlendOp>(alphaOp),
        .colorWriteMask = toVkFlags(writeMask),
    };
  }

  // Compare members in a fixed order.
  [[nodiscard]] auto identifier() const {
    return std::tie(enable, srcColorFactor, dstColorFactor, colorOp, srcAlphaFactor, dstAlphaFactor, alphaOp,
                    writeMask);
  }
  bool operator<(const PipelineColorBlendAttachmentState& other) const {
    return identifier() < other.identifier();
  }
  bool operator==(const PipelineColorBlendAttachmentState& other) const {
    return identifier() == other.identifier();
  }
};
} // namespace aur
// clang-format off
template <>
struct std::hash<aur::PipelineColorBlendAttachmentState> {
  size_t operator()(const aur::PipelineColorBlendAttachmentState& info) const noexcept {
    size_t seed = 0;
    hashCombine(seed, std::hash<bool>()(info.enable));
    hashCombine(seed, std::hash<std::underlying_type_t<aur::BlendFactor>>()(std::to_underlying(info.srcColorFactor)));
    hashCombine(seed, std::hash<std::underlying_type_t<aur::BlendFactor>>()(std::to_underlying(info.dstColorFactor)));
    hashCombine(seed, std::hash<std::underlying_type_t<aur::BlendOp>>()(std::to_underlying(info.colorOp)));
    hashCombine(seed, std::hash<std::underlying_type_t<aur::BlendFactor>>()(std::to_underlying(info.srcAlphaFactor)));
    hashCombine(seed, std::hash<std::underlying_type_t<aur::BlendFactor>>()(std::to_underlying(info.dstAlphaFactor)));
    hashCombine(seed, std::hash<std::underlying_type_t<aur::BlendOp>>()(std::to_underlying(info.alphaOp)));
    hashCombine(seed, std::hash<uint32_t>()(aur::toVkFlags(info.writeMask)));
    return seed;
  }
};
// clang-format on

namespace aur {
struct PipelineColorBlendStateCreateInfo {
  bool logicOpEnable{false};
  LogicOp logicOp{LogicOp::Copy}; // You might need to create this enum too
  std::vector<PipelineColorBlendAttachmentState> attachments;
  std::array<f32, 4> blendConstants{0.0f, 0.0f, 0.0f, 0.0f};

  [[nodiscard]] VkPipelineColorBlendStateCreateInfo toVk() const {
    vkAttachments_ = attachments | std::views::transform(&PipelineColorBlendAttachmentState::toVk) |
                     std::ranges::to<std::vector>();

    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = logicOpEnable,
        .logicOp = static_cast<VkLogicOp>(logicOp),
        .attachmentCount = static_cast<u32>(attachments.size()),
        .pAttachments = vkAttachments_.data(),
        .blendConstants = {blendConstants[0], blendConstants[1], blendConstants[2], blendConstants[3]},
    };
  }
  // Compare members in a fixed order.
  [[nodiscard]] auto identifier() const {
    return std::tie(logicOpEnable, logicOp, attachments, blendConstants);
  }
  bool operator<(const PipelineColorBlendStateCreateInfo& other) const {
    return identifier() < other.identifier();
  }
  bool operator==(const PipelineColorBlendStateCreateInfo& other) const {
    return identifier() == other.identifier();
  }

private:
  // Changing vkAttachments_ won't affect the const-ness of this CreateInfo. It's an implementation detail
  // used when creating a Pipeline to keep blend state info in memory. It does not contribute to the hash of
  // the CreateInfo.
  mutable std::vector<VkPipelineColorBlendAttachmentState> vkAttachments_;
};
} // namespace aur
// clang-format off
template <>
struct std::hash<aur::PipelineColorBlendStateCreateInfo> {
  size_t operator()(const aur::PipelineColorBlendStateCreateInfo& info) const noexcept {
    size_t seed = 0;
    hashCombine(seed, std::hash<bool>()(info.logicOpEnable));
    hashCombine(seed, std::hash<std::underlying_type_t<aur::LogicOp>>()(std::to_underlying(info.logicOp)));
    for (const auto& att : info.attachments)
      hashCombine(seed, std::hash<aur::PipelineColorBlendAttachmentState>()(att));
    hashCombine(seed, std::hash<aur::f32>()(info.blendConstants[0]));
    hashCombine(seed, std::hash<aur::f32>()(info.blendConstants[1]));
    hashCombine(seed, std::hash<aur::f32>()(info.blendConstants[2]));
    hashCombine(seed, std::hash<aur::f32>()(info.blendConstants[3]));
    return seed;
  }
};
// clang-format on

namespace aur {
struct PipelineCreateInfo {
  // Increment version after each change to the schema or processing logic
  static constexpr u32 schemaVersion{1};
  u32 version{schemaVersion};

  Handle<render::GraphicsProgram> graphicsProgram;
  PipelineRasterizationStateCreateInfo rasterizationState;
  PipelineDepthStencilStateCreateInfo depthStencilState;
  PipelineColorBlendStateCreateInfo colorBlendState;

  // Compare members in a fixed order.
  [[nodiscard]] auto identifier() const {
    return std::tie(graphicsProgram.id, rasterizationState, depthStencilState, colorBlendState);
  }
  bool operator<(const PipelineCreateInfo& other) const { return identifier() < other.identifier(); }
  bool operator==(const PipelineCreateInfo& other) const { return identifier() == other.identifier(); }
};
} // namespace aur
// clang-format off
template <>
struct std::hash<aur::PipelineCreateInfo> {
  size_t operator()(const aur::PipelineCreateInfo& info) const noexcept {
    size_t seed = 0;
    hashCombine(seed, std::hash<aur::u32>()(info.graphicsProgram.id));
    hashCombine(seed, std::hash<aur::PipelineRasterizationStateCreateInfo>()(info.rasterizationState));
    hashCombine(seed, std::hash<aur::PipelineDepthStencilStateCreateInfo>()(info.depthStencilState));
    hashCombine(seed, std::hash<aur::PipelineColorBlendStateCreateInfo>()(info.colorBlendState));
    return seed;
  }
};
// clang-format on

namespace aur {
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
