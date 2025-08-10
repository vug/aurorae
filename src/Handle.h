#pragma once

#include <utility>

#include "Utils.h"
// #include "asset/AssetConcepts.h"

namespace aur {

template <typename TAsset>
struct Handle {
  static constexpr u32 kInvalidId = static_cast<u32>(-1);
  Handle() = default;
  explicit Handle(u32 id)
      : id(id) {}

  u32 id{kInvalidId};
  [[nodiscard]] bool isValid() const { return id != kInvalidId; }

  const TAsset& get() const;
  const TAsset* operator->() const { return &get(); }
  const TAsset& operator*() const { return get(); }

  bool operator==(const Handle<TAsset>& other) const { return id == other.id; }
  // for using as a key in maps
  bool operator<(const Handle<TAsset>& other) const { return id < other.id; }
  // Intentional implicit conversion: Handle can be used as u32 where needed
  operator u32() const { return id; } // NOLINT(google-explicit-constructor)
};
} // namespace aur

template <typename TAsset>
struct std::hash<aur::Handle<TAsset>> {
  size_t operator()(const aur::Handle<TAsset>& handle) const noexcept {
    return std::hash<aur::u32>{}(handle.id);
  }
}; // namespace std
