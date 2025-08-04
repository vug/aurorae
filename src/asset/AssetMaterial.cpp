#include "Material.h"

namespace aur::asset {
Material Material::create(Handle<GraphicsProgram> graphProg) {
  Material material;
  material.graphicsProgram_ = graphProg;
  return material;
}
} // namespace aur::asset
