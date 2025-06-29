#pragma once

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

// C:/Users/veliu/repos/aurorae/src/assets
inline constexpr const char* kAssetsFolder{ASSETS_FOLDER};

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
inline constexpr const char* kShadersFolder{ASSETS_FOLDER "/shaders/release"};
#elif defined(REL_WITH_DEBUG_INFO_BUILD)
constexpr BuildType kBuildType{BuildType::RelWithDebInfo};
inline constexpr const char* kShadersFolder{ASSETS_FOLDER "/shaders/relwithdebinfo"};
#else
constexpr BuildType kBuildType{BuildType::Release};
inline constexpr const char* kShadersFolder = {ASSETS_FOLDER "/shaders/release"};
#endif
inline constexpr const char* kModelsFolder{ASSETS_FOLDER "/models"};

class PathBuffer {
public:
  explicit PathBuffer(char* data);
  ~PathBuffer();
  PathBuffer(const PathBuffer&) = delete;
  PathBuffer& operator=(const PathBuffer&) = delete;
  PathBuffer(PathBuffer&& other) noexcept;
  PathBuffer& operator=(PathBuffer&& other) noexcept;
  const char* c_str() const;
  explicit operator const char*() const;

private:
  char* data_;
};

PathBuffer pathJoin(const char* path, const char* relativeSubpath);

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