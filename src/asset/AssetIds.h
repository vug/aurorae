#pragma once

#include <glaze/glaze/glaze.hpp>
#include <modern-uuid/uuid.h>

#include "AssetConcepts.h"

namespace aur {

// --- Asset UUID ---
// Wrapper class to overcome the glaze issue https://github.com/stephenberry/glaze/issues/1477
// When `to<JSON, NotConvertibleToStringType>` glaze put extra double quotes when the type is used as a key
// in a map To prevent that we derive a thin class that adds `operator std::string_view()` operator, and use
// that one when de/serialization from/to JSON. Also see something slightly relevant:
// https://github.com/stephenberry/glaze/issues/1483
struct glaze_uuid : muuid::uuid {
  using muuid::uuid::uuid;

  operator std::string_view() const {
    thread_local std::string_view buffer;
    buffer = this->to_string();
    return buffer;
  }

  glaze_uuid(const muuid::uuid& u)
      : muuid::uuid(u) {}
  // shouldn't be needed
  // operator muuid::uuid() const { return *this; }
  // glaze_uuid() = default;
};
using AssetUuid = glaze_uuid;

// --- Asset Stable Source Identifier ---
template <AssetDefinitionConcept TDefinition>
class StableId : public std::string {
public:
  using std::string::string;

  StableId(const std::string& s)
      : std::string(s) {}
  StableId(std::string_view s)
      : std::string(s) {}
};
} // namespace aur