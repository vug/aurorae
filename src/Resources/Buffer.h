#pragma once

#include "../Utils.h"
#include "../VulkanWrappers.h"
#include "VulkanResource.h"
#include <optional>

namespace aur {

struct BufferCreateInfo {
  u64 sizeBytes{};
  std::vector<BufferUsage> usages;
  MemoryUsage memoryUsage{};
  std::optional<u32> itemCnt;
};

class Buffer : public VulkanResource<Buffer, VkBuffer, BufferCreateInfo, VmaAllocator> {
public:
  Buffer() = default;
  Buffer(VmaAllocator allocator, const BufferCreateInfo& createInfo);
  ~Buffer();

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  Buffer(Buffer&& other) noexcept;
  Buffer& operator=(Buffer&& other) noexcept;

  [[nodiscard]] std::byte* map() const;
  [[nodiscard]] std::byte* getMapPtr() const;
  void unmap() const;

private:
  friend class VulkanResource<Buffer, VkBuffer, BufferCreateInfo, VmaAllocator>;

  // Static creation method for the handle
  static VkBuffer createImpl(Buffer* self, const BufferCreateInfo& createInfo,
                             const std::tuple<VmaAllocator>& context);

  void destroyImpl();

  VmaAllocation allocation_;
  std::byte* mapPtr_{};
};

} // namespace aur