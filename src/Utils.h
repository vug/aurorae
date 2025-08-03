#pragma once

#include <filesystem>

namespace aur {

// Signed integers
using i8 = signed char;
using i16 = short;
using i32 = int;
using i64 = long long;
// Unsigned integers
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
// Floating point
using f32 = float;
using f64 = double;

// e.g. C:/Users/veliu/repos/aurorae/src/assets
inline const std::filesystem::path kAssetsFolder{ASSETS_FOLDER};

enum class BuildType : u8 {
  Debug,
  Release,
  RelWithDebInfo,
};

// This is a hint for the IDE so that it won't think {ASSETS_FOLDER "/shaders/..."} is an error
#ifndef ASSETS_FOLDER
#define ASSETS_FOLDER "path/to/assets"
#endif

#if defined(DEBUG_BUILD)
constexpr BuildType kBuildType{BuildType::Debug};
inline constexpr const char* kShadersFolder{ASSETS_FOLDER "/shaders/debug"};
#elif defined(RELEASE_BUILD)
constexpr BuildType kBuildType{BuildType::Release};
#elif defined(REL_WITH_DEBUG_INFO_BUILD)
constexpr BuildType kBuildType{BuildType::RelWithDebInfo};
#else
constexpr BuildType kBuildType{BuildType::Release};
#endif
inline constexpr const char* kModelsFolder{ASSETS_FOLDER "/models"};

// In Utils.h - don't reference VkResult at all
const char* vkResultToString(i32 result);

// Helper macros for token pasting
#define PASTE_IMPL(a, b) a##b
#define PASTE(a, b) PASTE_IMPL(a, b)

#define VK(vk_call)                                                                                          \
  do {                                                                                                       \
    if (const VkResult PASTE(result_, __LINE__) = (vk_call); PASTE(result_, __LINE__) != VK_SUCCESS) {       \
      log().fatal("Vulkan call `{}` failed! {}", #vk_call, vkResultToString(PASTE(result_, __LINE__)));      \
    }                                                                                                        \
  } while (0)

} // namespace aur