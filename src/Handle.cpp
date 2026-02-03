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
#ifdef _DEBUG
  debugPtr_ = result;
  debugTypeName_ = typeid(THandleable).name();
#endif
  return *result;
}

template <HandleableConcept THandleable>
THandleable& Handle<THandleable>::get() {
  THandleable* result{};
  if constexpr (AssetConcept<THandleable>)
    result = AppContext::get<AssetManager>().get(*this);
  else if constexpr (RenderObjectConcept<THandleable>)
    result = AppContext::get<Renderer>().get(*this);
  else {
    log().fatal("Invalid asset type.");
    std::unreachable();
  }
#ifdef _DEBUG
  debugPtr_ = result;
  debugTypeName_ = typeid(THandleable).name();
#endif
  return *result;
}

template const asset::ShaderStage& Handle<asset::ShaderStage>::get() const;
template const asset::GraphicsProgram& Handle<asset::GraphicsProgram>::get() const;
template const asset::Material& Handle<asset::Material>::get() const;
template const asset::Mesh& Handle<asset::Mesh>::get() const;
template const render::GraphicsProgram& Handle<render::GraphicsProgram>::get() const;
template const render::Material& Handle<render::Material>::get() const;
template const render::Mesh& Handle<render::Mesh>::get() const;

template asset::ShaderStage& Handle<asset::ShaderStage>::get();
template asset::GraphicsProgram& Handle<asset::GraphicsProgram>::get();
template asset::Material& Handle<asset::Material>::get();
template asset::Mesh& Handle<asset::Mesh>::get();
template render::GraphicsProgram& Handle<render::GraphicsProgram>::get();
template render::Material& Handle<render::Material>::get();
template render::Mesh& Handle<render::Mesh>::get();

} // namespace aur