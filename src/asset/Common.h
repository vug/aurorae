#pragma once

#include <string>

namespace aur {

// Stable Source Identifier
template <typename TAssetDef>
class StableId : public std::string {
public:
  using std::string::string;

  StableId(const std::string& s)
      : std::string(s) {}
  StableId(std::string_view s)
      : std::string(s) {}
};

} // namespace aur

namespace std {
template <typename TAssetDef>
struct hash<aur::StableId<TAssetDef>> {
  size_t operator()(const aur::StableId<TAssetDef>& stableId) const noexcept {
    // Use the hash of the underlying string
    return std::hash<std::string>{}(static_cast<const std::string&>(stableId));
  }
};
} // namespace std
