#pragma once

#include "Utils.h"

namespace aur {

template <typename TAsset>
struct Handle {
  static constexpr u32 kInvalidId = static_cast<u32>(-1);
  Handle() = default;
  explicit Handle(u32 id)
      : id(id) {}

  u32 id{kInvalidId};
  bool isValid() const { return id != kInvalidId; }

  const TAsset& get() const;

  bool operator==(const Handle<TAsset>& other) const { return id == other.id; }
  // for using as a key in maps
  bool operator<(const Handle<TAsset>& other) const { return id < other.id; }
  operator u32() const { return id; }
};
} // namespace aur
