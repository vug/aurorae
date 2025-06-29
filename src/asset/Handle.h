#pragma once

#include "../Utils.h"

namespace aur {

template <typename TAsset>
struct Handle {
  // -1 represents an invalid handle
  u32 id = static_cast<u32>(-1);

  bool isValid() const { return id != static_cast<u32>(-1); }
  bool operator==(const Handle<TAsset>& other) const { return id == other.id; }
  // for using as a key in maps
  bool operator<(const Handle<TAsset>& other) const { return id < other.id; }
  operator u32() const { return id; }
};

} // namespace aur
