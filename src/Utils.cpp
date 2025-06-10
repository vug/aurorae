#include "Utils.h"

#include "Logger.h"

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

  size_t pathLen = strlen(path);
  size_t subLen = strlen(relativeSubpath);

  // +2: one for possible '/', one for '\0'
  size_t totalLen = pathLen + subLen + 2;
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

} // namespace aur