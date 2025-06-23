#pragma once

#include <vector>

#include "Resources/Buffer.h"
#include "Utils.h"
#include "Vertex.h"

namespace aur {

class Mesh {
public:
  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  Buffer vertexBuffer;
  Buffer indexBuffer;
};

} // namespace aur