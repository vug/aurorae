#include "Material.h"

namespace aur::asset {
Material Material::create(const MaterialDefinition& materialDef) {
  Material material;
  material.def_ = materialDef;
  return material;
}
} // namespace aur::asset
