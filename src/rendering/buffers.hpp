#pragma once

#include <vulkan/vulkan_core.h>
#include "common.hpp"
#include "vk_mem_alloc.h"

namespace vre::rendering {

struct CreateBufferInfo {
  VkDeviceSize buffer_size{0};

  VkBufferUsageFlags usage{0};
  VmaMemoryUsage memory_usage{VMA_MEMORY_USAGE_UNKNOWN};

  VmaPool pool;

  const void *initial_data = nullptr;
};

// TODO(dmitrygladky): destructors
class Buffer {
 public:
  Buffer(VkBuffer buffer, VmaAllocator vma_allocator, VmaAllocation vma_allocation, VkDeviceSize size,
         VmaAllocationInfo allocation_info)
      : buffer_(buffer),
        vma_allocator_(vma_allocator),
        vma_allocation_(vma_allocation),
        size_(size),
        allocation_info_(allocation_info) {}

  virtual ~Buffer() { vmaDestroyBuffer(vma_allocator_, buffer_, vma_allocation_); }

  void Update(const void *data) {
    VR_ASSERT(allocation_info_.pMappedData);
    memcpy(allocation_info_.pMappedData, data, size_);
  }

  [[nodiscard]] void *GetMappedData() const {
    VR_ASSERT(allocation_info_.pMappedData);
    return allocation_info_.pMappedData;
  }

  [[nodiscard]] VkDeviceSize GetSize() const { return size_; }
  [[nodiscard]] VkBuffer GetBuffer() const { return buffer_; }

  [[nodiscard]] VkDeviceSize GetBufferOffset() const { return allocation_info_.offset; }

 private:
  VkBuffer buffer_;
  VmaAllocator vma_allocator_;
  VmaAllocation vma_allocation_;
  VkDeviceSize size_;

  VmaAllocationInfo allocation_info_;
};

}  // namespace vre::rendering