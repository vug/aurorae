#include "Utils.h"

#include <cstdio>
#include <string>

#include "Logger.h"

namespace aur {

class FileHandle {
 public:
  explicit FileHandle(const std::string& filename, const char* mode);

  // Implicit conversion to FILE* for use with C APIs
  operator FILE*() const { return file_.get(); }

 private:
  struct Deleter {
    void operator()(FILE* f) const {
      if (f) fclose(f);
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

std::vector<char> readFile(const std::string& filename, const char* mode) {
  FileHandle file(filename, mode);

  // Seek to the end of the file to determine its size
  std::fseek(file, 0, SEEK_END);
  long fileSizeLong = std::ftell(file);
  if (fileSizeLong < 0)
    log().fatal("Failed to determine size of file (ftell failed): {}",
                filename);
  size_t fileSize = static_cast<size_t>(fileSizeLong);

  // Seek back to the beginning of the file
  std::fseek(file, 0, SEEK_SET);

  std::vector<char> buffer(fileSize);

  // Read the entire file into the buffer
  size_t bytesRead = std::fread(buffer.data(), 1, fileSize, file);

  if (bytesRead != fileSize)
    log().fatal("Failed to read the entire file (read {} of {} bytes): {}",
                bytesRead, fileSize, filename);

  return buffer;
}

}  // namespace aur