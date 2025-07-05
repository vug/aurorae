#include "Handle.h"

#include "AppContext.h"
#include "Renderer.h"
#include "asset/AssetManager.h"

namespace aur {

namespace asset {
class Mesh;
class Shader;
struct Material;
} // namespace asset

namespace render {
class Shader;
// class Mesh;
} // namespace render

template <typename TAsset>
const TAsset& Handle<TAsset>::get() const {
  if constexpr (std::is_same_v<TAsset, asset::Shader> || std::is_same_v<TAsset, asset::Material> ||
                std::is_same_v<TAsset, asset::Mesh>)
    return *AppContext::getConst<AssetManager>().get(*this);
  else if constexpr (std::is_same_v<TAsset, render::Shader> || std::is_same_v<TAsset, render::Mesh>)
    return *AppContext::getConst<Renderer>().get(*this);
}
template const asset::Shader& Handle<asset::Shader>::get() const;
template const asset::Material& Handle<asset::Material>::get() const;
template const asset::Mesh& Handle<asset::Mesh>::get() const;
template const render::Shader& Handle<render::Shader>::get() const;

} // namespace aur