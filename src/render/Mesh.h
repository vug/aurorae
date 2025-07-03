#pragma once

#include <string>

#include "../Handle.h"
#include "../Resources/Buffer.h"

namespace aur::asset {
struct Mesh;
}

namespace aur::render {

struct DrawSpan {
  // TODO(vug): bring render::Material
  // Handle<render::Material> material;
  u32 offset;
  u32 count;
};

struct Mesh {
  Buffer vertexBuffer;
  Buffer indexBuffer;
  std::string debugName;

  std::vector<DrawSpan> drawSpans;
  Handle<asset::Mesh> asset;
};

} // namespace aur::render