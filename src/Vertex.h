#pragma once

#include <array>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Utils.h"
#include "VulkanWrappers.h"

namespace aur {

struct VertexInputBindingDescription {
  u32 binding{0};
  u32 stride{};
  VertexInputRate inputRate{VertexInputRate::Vertex};
};

struct VertexInputAttributeDescription {
  u32 location{};
  u32 binding{};
  Format format{};
  u32 offset{};
};

struct Vertex {
  glm::vec3 position;
  glm::vec4 color;

  static std::array<VertexInputBindingDescription, 1> getVertexInputBindingDescription();
  static std::array<VertexInputAttributeDescription, 2> getVertexInputAttributeDescription();

  /*
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
      std::vector<VkVertexInputAttributeAttributeDescription> attributeDescriptions{};

      // Position attribute
      attributeDescriptions.push_back({});
      attributeDescriptions[0].binding = 0;
      attributeDescriptions[0].location = 0; // Corresponds to `layout(location = 0)` in shader
      attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // glm::vec3 is 3 floats
      attributeDescriptions[0].offset = offsetof(FatVertex, pos); // Offset within the vertex structure

      // Color attribute
      attributeDescriptions.push_back({});
      attributeDescriptions[1].binding = 0;
      attributeDescriptions[1].location = 1; // Corresponds to `layout(location = 1)` in shader
      attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; // glm::vec3 is 3 floats
      attributeDescriptions[1].offset = offsetof(FatVertex, color); // Offset within the vertex structure

      return attributeDescriptions;
    }
  */
};

inline std::array<VertexInputBindingDescription, 1> Vertex::getVertexInputBindingDescription() {
  return {VertexInputBindingDescription{
      .stride = sizeof(Vertex),
      .inputRate = VertexInputRate::Vertex,
  }};
}

inline std::array<VertexInputAttributeDescription, 2> Vertex::getVertexInputAttributeDescription() {
  return {
      VertexInputAttributeDescription{
          .location = 0,
          .binding = 0,
          .format = Format::R32G32B32_SFLOAT,
          .offset = offsetof(Vertex, position),
      },
      VertexInputAttributeDescription{
          .location = 1,
          .binding = 0,
          .format = Format::R32G32B32A32_SFLOAT,
          .offset = offsetof(Vertex, color),
      },
  };
}

} // namespace aur