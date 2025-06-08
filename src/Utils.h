#pragma once

namespace aur {

// Signed integers
using i8  = signed char;
using i16 = short;
using i32 = int;
using i64 = long long;
// Unsigned integers
using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
// Floating point
using f32 = float;
using f64 = double;

enum class BuildType : u8 {
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

}  // namespace aur
