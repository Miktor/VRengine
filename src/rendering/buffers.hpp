#pragma once

#include <vulkan/vulkan_core.h>
#include "common.hpp"
#include "vk_mem_alloc.h"

namespace vre {

class Application;

namespace rendering {

struct CreateBufferInfo {
  VkDeviceSize buffer_size{0};

  VkBufferUsageFlags usage{0};
  VmaMemoryUsage memory_usage{VMA_MEMORY_USAGE_UNKNOWN};

  const void *initial_data = nullptr;
};

// TODO(dmitrygladky): destructors
class Buffer {
 public:
  static constexpr VkBufferUsageFlagBits kBufferBit = static_cast<VkBufferUsageFlagBits>(0);

  Buffer(VkBuffer buffer, VmaAllocator vma_allocator, VmaAllocation vma_allocation, VkDeviceSize size,
         VmaAllocationInfo allocation_info)
      : buffer_(buffer),
        vma_allocator_(vma_allocator),
        vma_allocation_(vma_allocation),
        size_(size),
        allocation_info_(allocation_info) {}

  void Update(const void *data) {
    VR_ASSERT(allocation_info_.pMappedData);
    memcpy(allocation_info_.pMappedData, data, size_);
  }

  [[nodiscard]] VkDeviceSize GetSize() const { return size_; }
  [[nodiscard]] VkBuffer GetBuffer() const { return buffer_; }

 private:
  VkBuffer buffer_;
  VmaAllocator vma_allocator_;
  VmaAllocation vma_allocation_;
  VkDeviceSize size_;

  VmaAllocationInfo allocation_info_;
};

}  // namespace rendering
}  // namespace vre