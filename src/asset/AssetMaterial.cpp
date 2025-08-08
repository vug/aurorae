#include "Material.h"

namespace aur::asset {

Material Material::create(MaterialDefinition&& matDef, Handle<GraphicsProgram> graphProg) {
  Material material;
  material.graphicsProgram_ = graphProg;
  material.materialDef_ = std::move(matDef);
  return material;
}

} // namespace aur::asset
