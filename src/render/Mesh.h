#pragma once

#include <string>

#include "../Resources/Buffer.h"

namespace aur::render {

struct Mesh {
  Buffer vertexBuffer;
  Buffer indexBuffer;
  std::string debugName;
};

} // namespace aur::render