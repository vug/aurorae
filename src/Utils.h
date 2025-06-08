#pragma once

#include <vector>

typedef unsigned char uint8_t;

// Forward declare std::string. Lol.
namespace std {
  template<typename T1, typename T2, typename T3>
  class basic_string;
  template<typename T>
  struct char_traits;
  template<typename T>
  class allocator;
  using string  = basic_string<char, char_traits<char>, allocator<char>>;
}

namespace aur {

enum class BuildType : uint8_t {
  Debug,
  Release,
  RelWithDebInfo,
};

constexpr BuildType kBuildType =
#if defined(DEBUG_BUILD)
    BuildType::Debug;
#elif defined(RELEASE_BUILD)
    BuildType::Release;
#elif defined(REL_WITH_DEBUG_INFO_BUILD)
    BuildType::RelWithDebInfo;
#else
    BuildType::Release;
#endif

// Utility function to read a binary file
std::vector<char> readFile(const std::string& filename, const char* mode);

}  // namespace aur
