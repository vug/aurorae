#pragma once

typedef unsigned char uint8_t;

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

}  // namespace aur
