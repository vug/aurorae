#include "AssetIds.h"

#include "../Logger.h"
#include "AssetRegistry.h"

namespace aur {
AssetRef::operator AssetUuid() const {
  if (mode_ == Mode::Invalid)
    log().fatal("AssetRef uninitialized");

  if (mode_ == Mode::AssetUuid)
    return uuid_;

  if (!registry_)
    log().fatal("Cannot convert to UUID without registry context");

  auto uuidOpt = registry_->getUuid(stableId_);
  if (!uuidOpt)
    log().fatal("Cannot resolve stable ID {} to UUID.", stableId_);

  return *uuidOpt;
}

AssetRef::operator std::string() const {
  if (mode_ == Mode::Invalid)
    log().fatal("AssetRef uninitialized");

  if (mode_ == Mode::StableId)
    return stableId_;

  if (!registry_)
    log().fatal("Cannot convert to StableId without registry context");

  auto entryOpt = registry_->getEntry(uuid_);
  if (!entryOpt)
    log().fatal("Cannot resolve UUID {} to stable Id", uuid_.to_chars());
  return stableId_;
}

} // namespace aur