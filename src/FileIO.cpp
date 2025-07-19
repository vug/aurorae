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
  operator FILE*() const { return file_.get(); } // NOLINT(google-explicit-constructor)

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
template <typename TItem>
std::vector<TItem> readBinaryFile(const std::filesystem::path& filePath) {
  static_assert(std::is_trivial_v<TItem> && std::is_standard_layout_v<TItem>,
                "Only trivial types supported for binary file reading");

  const FileHandle file(filePath.string(), "rb");

  // Seek to the end of the file to determine its size
  if (std::fseek(file, 0, SEEK_END) != 0)
    log().fatal("Failed to seek to end of the file: {}", filePath.string());

  const long fileSizeLong = std::ftell(file);
  if (fileSizeLong < 0)
    log().fatal("Failed to determine size of file (ftell failed): {}", filePath.string());

  if (fileSizeLong % sizeof(TItem) != 0)
    log().fatal("File '{}' size is not a multiple of type size.", filePath.string());

  // Seek back to the beginning of the file
  if (std::fseek(file, 0, SEEK_SET) != 0)
    log().fatal("Failed to seek to beginning of the file: {}", filePath.string());

  const auto fileSizeBytes = static_cast<size_t>(fileSizeLong);
  std::vector<TItem> buffer(fileSizeBytes / sizeof(TItem));

  if (const size_t bytesRead = std::fread(buffer.data(), 1, fileSizeBytes, file); bytesRead != fileSizeBytes)
    log().fatal("Failed to read the entire file (read {} of {} bytes): {}", bytesRead, fileSizeBytes,
                filePath.string());

  return buffer;
}

template std::vector<std::byte> readBinaryFile<std::byte>(const std::filesystem::path& filePath);
template std::vector<u32> readBinaryFile<u32>(const std::filesystem::path& filePath);

bool writeBinaryFile(const std::filesystem::path& filePath, const void* data, size_t sizeBytes) {
  if (!data || sizeBytes == 0) {
    log().warn("Cannot write empty data to file: {}", filePath.string());
    return false;
  }

  // create non-existent folders in the path
  const auto parentPath = filePath.parent_path();
  if (!parentPath.empty()) {
    if (std::error_code ec; !std::filesystem::create_directories(parentPath, ec) && ec) {
      log().error("Failed to create directories for path: {}, error: {}", parentPath.string(), ec.message());
      return false;
    }
  }

  const FileHandle file(filePath.string(), "wb");

  const size_t bytesWritten = std::fwrite(data, 1, sizeBytes, file);
  if (bytesWritten != sizeBytes) {
    log().error("Failed to write the entire file (wrote {} of {} bytes): {}", bytesWritten, sizeBytes,
                filePath.string());
    return false;
  }

  return true;
}

} // namespace aur