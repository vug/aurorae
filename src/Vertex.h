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
  glm::vec3 normal;
  glm::vec3 tangent; // xyz = tangent, w = bitangent handedness
  // glm::vec3 bitangent; // Can be computed in shader from normal + tangent
  glm::vec4 color;
  glm::vec2 texCoord0; // Material textures
  glm::vec2 texCoord1; // Lightmaps
  glm::vec2 texCoord2; // Detail textures, decals, or procedural UVs

  glm::vec4 custom0;
  glm::vec4 custom1;

  // // Future-proofing additions:
  // glm::vec4 joints;       // Bone indices for skeletal animation
  // glm::vec4 weights;      // Bone weights for skeletal animation
  // glm::vec3 morphTarget0; // Position delta for morph targets
  // glm::vec3 morphTarget1; // Normal delta for morph targets

  static std::array<VertexInputBindingDescription, 1> getVertexInputBindingDescription();
  static std::array<VertexInputAttributeDescription, 9> getVertexInputAttributeDescription();
};

inline std::array<VertexInputBindingDescription, 1> Vertex::getVertexInputBindingDescription() {
  return {VertexInputBindingDescription{
      .stride = sizeof(Vertex),
      .inputRate = VertexInputRate::Vertex,
  }};
}

inline std::array<VertexInputAttributeDescription, 9> Vertex::getVertexInputAttributeDescription() {
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
          .format = Format::R32G32B32_SFLOAT,
          .offset = offsetof(Vertex, normal),
      },
      VertexInputAttributeDescription{
          .location = 2,
          .binding = 0,
          .format = Format::R32G32B32_SFLOAT,
          .offset = offsetof(Vertex, tangent),
      },
      VertexInputAttributeDescription{
          .location = 3,
          .binding = 0,
          .format = Format::R32G32B32A32_SFLOAT,
          .offset = offsetof(Vertex, color),
      },
      VertexInputAttributeDescription{
          .location = 4,
          .binding = 0,
          .format = Format::R32G32_SFLOAT,
          .offset = offsetof(Vertex, texCoord0),
      },
      VertexInputAttributeDescription{
          .location = 5,
          .binding = 0,
          .format = Format::R32G32_SFLOAT,
          .offset = offsetof(Vertex, texCoord1),
      },
      VertexInputAttributeDescription{
          .location = 6,
          .binding = 0,
          .format = Format::R32G32_SFLOAT,
          .offset = offsetof(Vertex, texCoord2),
      },
      VertexInputAttributeDescription{
          .location = 7,
          .binding = 0,
          .format = Format::R32G32B32A32_SFLOAT,
          .offset = offsetof(Vertex, custom0),
      },
      VertexInputAttributeDescription{
          .location = 8,
          .binding = 0,
          .format = Format::R32G32B32A32_SFLOAT,
          .offset = offsetof(Vertex, custom1),
      },

  };
}

} // namespace aur