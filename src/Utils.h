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
inline constexpr const char* kShadersFolder{ASSETS_FOLDER "/shaders/release"};
#endif

class PathBuffer {
 public:
  explicit PathBuffer(char* data);
  ~PathBuffer();
  PathBuffer(const PathBuffer&) = delete;
  PathBuffer& operator=(const PathBuffer&) = delete;
  PathBuffer(PathBuffer&& other) noexcept;
  PathBuffer& operator=(PathBuffer&& other) noexcept;
  const char* c_str() const;
  operator const char*() const;

 private:
  char* data_;
};

PathBuffer pathJoin(const char* path, const char* relativeSubpath);

}  // namespace aur
