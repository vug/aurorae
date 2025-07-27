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