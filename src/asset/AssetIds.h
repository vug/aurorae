#pragma once

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

class AssetRegistry;

// AssetRef can hold either a UUID or a string and converts between them automatically
class AssetRef {
public:
  enum class Mode {
    Invalid,
    AssetUuid,
    StableId,
  };

  // Constructors
  AssetRef() = default;
  explicit AssetRef(const AssetUuid& uuid)
      : uuid_(uuid)
      , mode_(Mode::AssetUuid) {}
  explicit AssetRef(const std::string& stableId)
      : stableId_(stableId)
      , mode_(Mode::StableId) {}
  explicit AssetRef(std::string_view stableId)
      : stableId_(stableId)
      , mode_(Mode::StableId) {}

  operator AssetUuid() const;
  operator std::string() const;

  // Getters
  const AssetUuid& getUuid() const { return uuid_; }
  template <AssetDefinitionConcept TDefinition>
  const StableId<TDefinition>& getStableId() const {
    return stableId_;
  }

  // Context injection for registry access
  void setRegistry(const AssetRegistry* registry) { registry_ = registry; }

  bool hasUuid() const { return mode_ == Mode::AssetUuid; }
  bool hasStableId() const { return mode_ == Mode::StableId; }

private:
  AssetUuid uuid_;
  std::string stableId_;
  Mode mode_{Mode::Invalid};

  // Context for conversion - injected during serialization
  mutable const AssetRegistry* registry_ = nullptr;

  // friend class AssetRefSerializer; // For Glaze integration
};
} // namespace aur

namespace glz {
// JSON & BEVE ser/de for UUID
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

template <>
struct from<BEVE, aur::glaze_uuid> {
  template <auto Opts>
  static void op(aur::glaze_uuid& uuid, auto&&... args) {
    std::string str;
    parse<BEVE>::op<Opts>(str, args...);
    uuid = muuid::uuid::from_chars(str).value();
  }
};

template <>
struct to<BEVE, aur::glaze_uuid> {
  template <auto Opts>
  static void op(const aur::glaze_uuid& uuid, auto&&... args) noexcept {
    std::string str = uuid.to_string();
    serialize<BEVE>::op<Opts>(str, args...);
  }
};

// JSON serialization for AssetRef (stores as string - stable ID)
template <>
struct from<JSON, aur::AssetRef> {
  template <auto Opts>
  static void op(aur::AssetRef& ref, auto&&... args) {
    std::string stableId;
    parse<JSON>::op<Opts>(stableId, args...);
    ref = aur::AssetRef(stableId);
  }
};

template <>
struct to<JSON, aur::AssetRef> {
  template <auto Opts>
  static void op(const aur::AssetRef& ref, auto&&... args) noexcept {
    std::string stableId = ref; // Uses conversion operator
    serialize<JSON>::op<Opts>(stableId, args...);
  }
};

// BEVE serialization for AssetRef (stores as UUID for binary format)
template <>
struct from<BEVE, aur::AssetRef> {
  template <auto Opts>
  static void op(aur::AssetRef& ref, auto&&... args) {
    aur::AssetUuid uuid;
    parse<BEVE>::op<Opts>(uuid, args...);
    ref = aur::AssetRef(uuid);
  }
};

template <>
struct to<BEVE, aur::AssetRef> {
  template <auto Opts>
  static void op(const aur::AssetRef& ref, auto&&... args) noexcept {
    aur::AssetUuid uuid = ref; // Uses conversion operator
    serialize<BEVE>::op<Opts>(uuid, args...);
  }
};

} // namespace glz