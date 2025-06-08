#pragma once

namespace std {
// Forward declare std::string. Lol.
template <typename T>
struct char_traits;
template<
    class CharT,
    class Traits
> class basic_string_view;
using string_view = basic_string_view<char, std::char_traits<char>>;

enum class byte : unsigned char;
}  // namespace std

namespace aur {

class BinaryBlob {
public:
  explicit BinaryBlob(size_t size);
  ~BinaryBlob();
  
  // Move-only
  BinaryBlob(BinaryBlob&& other) noexcept;
  BinaryBlob& operator=(BinaryBlob&& other) noexcept;
  BinaryBlob(const BinaryBlob&) = delete;
  BinaryBlob& operator=(const BinaryBlob&) = delete;

  const std::byte* data() const { return data_; }
  std::byte* data() { return data_; }
  size_t size() const { return size_; }

private:
  std::byte* data_;
  size_t size_;
};  

// Utility function to read a binary file
BinaryBlob readBinaryFile(std::string_view filename);

}  // namespace aur