#include "Material.h"

#include "../Pipeline.h"
#include "../Renderer.h"
#include "GraphicsProgram.h"

namespace aur::render {

Material::Material(Renderer& renderer, Handle<asset::Material> asset)
    : renderer_{&renderer}
    , assetHandle_{asset}
    , graphicsProgramHandle_{[this]() {
      const asset::Material& aMaterial = assetHandle_.get();
      return renderer_->uploadOrGet(aMaterial.getGraphicsProgramHandle());
    }()} {}

} // namespace aur::render