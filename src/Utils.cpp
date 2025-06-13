#include "Utils.h"

#include "Logger.h"

#include <vulkan/vulkan_core.h>

namespace aur {

//----------------------------------------------------------------

PathBuffer::PathBuffer(char* data)
    : data_(data) {}

PathBuffer::~PathBuffer() {
  delete[] data_;
}

PathBuffer::PathBuffer(PathBuffer&& other) noexcept
    : data_(other.data_) {
  other.data_ = nullptr;
}

PathBuffer& PathBuffer::operator=(PathBuffer&& other) noexcept {
  if (this != &other) {
    delete[] data_;
    data_ = other.data_;
    other.data_ = nullptr;
  }
  return *this;
}

const char* PathBuffer::c_str() const {
  return data_;
}
PathBuffer::operator const char*() const {
  return data_;
}

//----------------------------------------------------------------

PathBuffer pathJoin(const char* path, const char* relativeSubpath) {
  if (!path || !relativeSubpath)
    return PathBuffer{nullptr};

  auto strlen = [](const char* str) -> size_t {
    size_t len = 0;
    while (str && str[len] != '\0')
      ++len;
    return len;
  };

  const size_t pathLen = strlen(path);
  const size_t subLen = strlen(relativeSubpath);

  // +2: one for possible '/', one for '\0'
  const size_t totalLen = pathLen + subLen + 2;
  char* outBuffer = new char[totalLen];

  size_t pos = 0;
  for (size_t i = 0; i < pathLen; ++i)
    outBuffer[pos++] = path[i];

  // Add '/' if needed
  if (pathLen > 0 && outBuffer[pos - 1] != '/')
    outBuffer[pos++] = '/';

  for (size_t i = 0; i < subLen; ++i)
    outBuffer[pos++] = relativeSubpath[i];

  outBuffer[pos] = '\0';
  return PathBuffer{outBuffer};
}

//----------------------------------------------------------------
const char* vkResultToString(i32 result) {
  switch (static_cast<VkResult>(result)) {
  case VK_SUCCESS:
    return "VK_SUCCESS";
  case VK_NOT_READY:
    return "VK_NOT_READY";
  case VK_TIMEOUT:
    return "VK_TIMEOUT";
  case VK_EVENT_SET:
    return "VK_EVENT_SET";
  case VK_EVENT_RESET:
    return "VK_EVENT_RESET";
  case VK_INCOMPLETE:
    return "VK_INCOMPLETE";
  case VK_ERROR_OUT_OF_HOST_MEMORY:
    return "VK_ERROR_OUT_OF_HOST_MEMORY";
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
  case VK_ERROR_INITIALIZATION_FAILED:
    return "VK_ERROR_INITIALIZATION_FAILED";
  case VK_ERROR_DEVICE_LOST:
    return "VK_ERROR_DEVICE_LOST";
  case VK_ERROR_MEMORY_MAP_FAILED:
    return "VK_ERROR_MEMORY_MAP_FAILED";
  case VK_ERROR_LAYER_NOT_PRESENT:
    return "VK_ERROR_LAYER_NOT_PRESENT";
  case VK_ERROR_EXTENSION_NOT_PRESENT:
    return "VK_ERROR_EXTENSION_NOT_PRESENT";
  case VK_ERROR_FEATURE_NOT_PRESENT:
    return "VK_ERROR_FEATURE_NOT_PRESENT";
  case VK_ERROR_INCOMPATIBLE_DRIVER:
    return "VK_ERROR_INCOMPATIBLE_DRIVER";
  case VK_ERROR_TOO_MANY_OBJECTS:
    return "VK_ERROR_TOO_MANY_OBJECTS";
  case VK_ERROR_FORMAT_NOT_SUPPORTED:
    return "VK_ERROR_FORMAT_NOT_SUPPORTED";
  case VK_ERROR_FRAGMENTED_POOL:
    return "VK_ERROR_FRAGMENTED_POOL";
  case VK_ERROR_UNKNOWN:
    return "VK_ERROR_UNKNOWN";
    // Add more cases as needed for extensions
  default:
    return "UNKNOWN_VK_RESULT";
  }
}

} // namespace aur