#include "FileIO.h"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <string>

#include "Logger.h"

namespace aur {

//----------------------------------------------------------------

class FileHandle {
public:
  explicit FileHandle(const std::string& filename, const char* mode);

  // Implicit conversion to FILE* for use with C APIs
  operator FILE*() const { return file_.get(); }

private:
  struct Deleter {
    void operator()(FILE* f) const {
      if (f)
        fclose(f);
    }
  };
  std::unique_ptr<FILE, Deleter> file_;
};

FileHandle::FileHandle(const std::string& filename, const char* mode) {
  FILE* rawFile{};
  errno_t err = fopen_s(&rawFile, filename.c_str(), mode);
  if (err != 0 || !rawFile)
    log().fatal("Failed to open file: {}, error no: {}", filename, err);
  file_.reset(rawFile);
}

//----------------------------------------------------------------

BinaryBlob::BinaryBlob(size_t size)
    : size_(size) {
  data_ = new std::byte[size];
}

BinaryBlob::~BinaryBlob() {
  delete[] data_;
}

BinaryBlob::BinaryBlob(BinaryBlob&& other) noexcept
    : data_(other.data_)
    , size_(other.size_) {
  other.data_ = nullptr;
  other.size_ = 0;
}

BinaryBlob& BinaryBlob::operator=(BinaryBlob&& other) noexcept {
  if (this != &other) {
    delete[] data_;
    data_ = other.data_;
    size_ = other.size_;
    other.data_ = nullptr;
    other.size_ = 0;
  }
  return *this;
}

//----------------------------------------------------------------

BinaryBlob readBinaryFile(std::string_view filename) {
  const FileHandle file(filename.data(), "rb");

  // Seek to the end of the file to determine its size
  std::fseek(file, 0, SEEK_END);
  const long fileSizeLong = std::ftell(file);
  if (fileSizeLong < 0)
    log().fatal("Failed to determine size of file (ftell failed): {}", filename);

  // Seek back to the beginning of the file
  std::fseek(file, 0, SEEK_SET);

  const size_t fileSize = static_cast<size_t>(fileSizeLong);
  BinaryBlob buffer(fileSize);

  assert(buffer.size() == fileSize);
  const size_t bytesRead = std::fread(buffer.data(), 1, fileSize, file);

  if (bytesRead != fileSize)
    log().fatal("Failed to read the entire file (read {} of {} bytes): {}", bytesRead, fileSize, filename);

  return buffer;
}
std::vector<std::byte> readBinaryFile(const std::filesystem::path& filePath) {
  const FileHandle file(filePath.string(), "rb");

  // Seek to the end of the file to determine its size
  if (std::fseek(file, 0, SEEK_END) != 0)
    log().fatal("Failed to seek to end of the file: {}", filePath.string());

  const long fileSizeLong = std::ftell(file);
  if (fileSizeLong < 0)
    log().fatal("Failed to determine size of file (ftell failed): {}", filePath.string());

  // Seek back to the beginning of the file
  if (std::fseek(file, 0, SEEK_SET) != 0)
    log().fatal("Failed to seek to beginning of the file: {}", filePath.string());

  const size_t fileSizeBytes = static_cast<size_t>(fileSizeLong);
  std::vector<std::byte> buffer(fileSizeBytes);

  const size_t bytesRead = std::fread(buffer.data(), 1, fileSizeBytes, file);

  if (bytesRead != fileSizeBytes)
    log().fatal("Failed to read the entire file (read {} of {} bytes): {}", bytesRead, fileSizeBytes,
                filePath.string());

  return buffer;
}

} // namespace aur