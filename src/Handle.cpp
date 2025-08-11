#include "Handle.h"

#include "AppContext.h"
#include "Renderer.h"
#include "asset/AssetManager.h"

namespace aur {

namespace asset {
class Mesh;
class GraphicsProgram;
class Material;
} // namespace asset

namespace render {
class GraphicsProgram;
class Mesh;
} // namespace render

template <HandleableConcept THandleable>
const THandleable& Handle<THandleable>::get() const {
  const THandleable* result{};
  if constexpr (AssetConcept<THandleable>)
    result = AppContext::getConst<AssetManager>().get(*this);
  else if constexpr (RenderObjectConcept<THandleable>)
    result = AppContext::getConst<Renderer>().get(*this);
  else {
    log().fatal("Invalid asset type.");
    std::unreachable();
  }
}
template const asset::ShaderStage& Handle<asset::ShaderStage>::get() const;
template const asset::GraphicsProgram& Handle<asset::GraphicsProgram>::get() const;
template const asset::Material& Handle<asset::Material>::get() const;
template const asset::Mesh& Handle<asset::Mesh>::get() const;
template const render::GraphicsProgram& Handle<render::GraphicsProgram>::get() const;
template const render::Material& Handle<render::Material>::get() const;
template const render::Mesh& Handle<render::Mesh>::get() const;

} // namespace aur