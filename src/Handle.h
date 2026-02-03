#pragma once

#include <utility>

#include "Utils.h"
#include "asset/AssetConcepts.h"
#include "render/RenderConcepts.h"

namespace aur {

template <typename THandleable>
concept HandleableConcept = AssetConcept<THandleable> || RenderObjectConcept<THandleable>;

template <HandleableConcept THandleable>
struct Handle {
  static constexpr u32 kInvalidId = static_cast<u32>(-1);
  Handle() = default;
  explicit Handle(u32 id)
      : id(id)
#ifdef _DEBUG
      , debugPtr_(&get())
      , debugTypeName_(typeid(THandleable).name())
#endif
  {
  }

  u32 id{kInvalidId};
  [[nodiscard]] bool isValid() const { return id != kInvalidId; }

  const THandleable& get() const;
  const THandleable* operator->() const { return &get(); }
  const THandleable& operator*() const { return get(); }

  THandleable& get();
  THandleable* operator->() { return &get(); }
  THandleable& operator*() { return get(); }

  bool operator==(const Handle<THandleable>& other) const { return id == other.id; }
  // for using as a key in maps
  bool operator<(const Handle<THandleable>& other) const { return id < other.id; }
  // Intentional implicit conversion: Handle can be used as u32 where needed
  operator u32() const { return id; } // NOLINT(google-explicit-constructor)

#ifdef _DEBUG // Or whatever debug macro you use
private:
  mutable const THandleable* debugPtr_{};
  mutable const char* debugTypeName_{};
#endif
};
} // namespace aur

template <typename THandleable>
struct std::hash<aur::Handle<THandleable>> {
  size_t operator()(const aur::Handle<THandleable>& handle) const noexcept {
    return std::hash<aur::u32>{}(handle.id);
  }
}; // namespace std
