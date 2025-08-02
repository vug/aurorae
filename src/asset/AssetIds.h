#pragma once

#include <glaze/glaze/glaze.hpp>
#include <modern-uuid/uuid.h>

#include "../Utils.h"

namespace aur {
enum class DefinitionType : u32 {
  ShaderStage = 0,
  GraphicsProgram = 1,
  Material = 2,
  Mesh = 3,
};

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
} // namespace aur

namespace std {
template <>
struct hash<aur::glaze_uuid> {
  size_t operator()(const aur::glaze_uuid& uuid) const noexcept { return std::hash<muuid::uuid>()(uuid); }
};
} // namespace std

namespace glz {
template <>
struct from<JSON, aur::glaze_uuid> {
  template <auto Opts>
  static void op(aur::glaze_uuid& uuid, auto&&... args) {
    // Initialize a string_view with the appropriately sized buffer
    // Alternatively, use a std::string for any size (but this will allocate)
    std::string_view str = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    parse<JSON>::op<Opts>(str, args...);
    uuid = muuid::uuid::from_chars(str).value();
  }
};

template <>
struct to<JSON, aur::glaze_uuid> {
  template <auto Opts>
  static void op(const aur::glaze_uuid& uuid, auto&&... args) noexcept {
    std::string str = uuid.to_string();
    serialize<JSON>::op<Opts>(str, args...);
  }
};
} // namespace glz

template <>
struct glz::meta<aur::DefinitionType> {
  using enum aur::DefinitionType;
  static constexpr auto value = glz::enumerate(ShaderStage, GraphicsProgram, Material, Mesh);
};

namespace aur {
using AssetUuid = glaze_uuid;

// -----------

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