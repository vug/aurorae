#include "Handle.h"

#include "AppContext.h"
#include "asset/AssetManager.h"

namespace aur {

namespace asset {
class Mesh;
class Shader;
struct Material;
} // namespace asset

template <typename TAsset>
const TAsset& Handle<TAsset>::get() const {
  return *AppContext::getConst<AssetManager>().get(*this);
}
template const asset::Mesh& Handle<asset::Mesh>::get() const;
template const asset::Shader& Handle<asset::Shader>::get() const;
template const asset::Material& Handle<asset::Material>::get() const;

} // namespace aur