#include "Material.h"

#include "../Pipeline.h"
#include "../Renderer.h"
#include "Shader.h"

namespace aur::render {

Material::Material(Renderer& renderer, Handle<asset::Material> asset)
    : renderer_{&renderer}
    , assetHandle_{asset}
    , shaderHnd_{[this]() {
      const asset::Material& aMaterial = assetHandle_.get();
      return renderer_->upload(aMaterial.getShaderHandle());
    }()} {}

} // namespace aur::render