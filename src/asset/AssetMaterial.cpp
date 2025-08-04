#include "Material.h"

namespace aur::asset {
Material Material::create(MaterialDefinition&& materialDef) {
  Material material;
  material.def_ = std::move(materialDef);
  return material;
}
} // namespace aur::asset
