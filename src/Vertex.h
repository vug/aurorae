#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace aur {

struct Vertex {
  glm::vec3 position;
  glm::vec4 color;
  /*
    static VkVertexInputBindingDescription getBindingDescription() {
      VkVertexInputBindingDescription bindingDescription{};
      bindingDescription.binding = 0; // The index of the binding in the array of bindings
      bindingDescription.stride = sizeof(FatVertex); // The byte stride between consecutive vertices
      bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to the next vertex after each vertex

      return bindingDescription;
    }

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
      attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // glm::vec3 is 3 floats
      attributeDescriptions[1].offset = offsetof(FatVertex, color); // Offset within the vertex structure

      return attributeDescriptions;
    }
  */
};

} // namespace aur