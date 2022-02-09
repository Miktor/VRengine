#include "uniform_buffer_allocator.hpp"

#include "helpers.hpp"
#include "rendering/render_core.hpp"
#include "vk_mem_alloc.h"

namespace vre::rendering {

namespace {

uint32_t GetVmaMemoryTypeIndex(VmaAllocator allocator) {
  VkBufferCreateInfo example_buf_create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  example_buf_create_info.size = 1024;  // Whatever.
  example_buf_create_info.usage =
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;  // Change if needed.

  VmaAllocationCreateInfo alloc_create_info = {};
  alloc_create_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;  // Change if needed.

  uint32_t mem_type_index;
  CHECK_VK_SUCCESS(vmaFindMemoryTypeIndexForBufferInfo(allocator, &example_buf_create_info,
                                                       &alloc_create_info, &mem_type_index));

  return mem_type_index;
}

}  // namespace

const Buffer &UniformBufferAllocation::GetBuffer() const { return *buffer_; }

UBOAllocation UniformBufferAllocation::Allocate(VkDeviceSize size) {
  const auto aligned_offset = (offset_ + alignment_ - 1) & ~(alignment_ - 1);
  VR_CHECK(aligned_offset + size <= size_);
  offset_ = aligned_offset + size;

  return {aligned_offset, size};
}

UniformBufferAllocation::UniformBufferAllocation(std::shared_ptr<Buffer> buffer, VkDeviceSize alignment,
                                                 VkDeviceSize size)
    : buffer_(std::move(buffer)), alignment_(alignment), size_(size) {}

UniformBufferPoolAllocator::UniformBufferPoolAllocator(RenderCore &render_core, VkDeviceSize block_size,
                                                       VkDeviceSize alignment, VkBufferUsageFlags usage)
    : render_core_(render_core), block_size_(block_size), alignment_(alignment), usage_(usage) {
  VmaPoolCreateInfo pool_create_info{};
  pool_create_info.memoryTypeIndex = GetVmaMemoryTypeIndex(render_core_.GetVmaAllocator());
  pool_create_info.blockSize = block_size_;
  pool_create_info.minBlockCount = 2;
  pool_create_info.minAllocationAlignment = alignment_;

  CHECK_VK_SUCCESS(vmaCreatePool(render_core_.GetVmaAllocator(), &pool_create_info, &pool_));
}

UniformBufferPoolAllocator::~UniformBufferPoolAllocator() {
  vmaDestroyPool(render_core_.GetVmaAllocator(), pool_);
}

std::shared_ptr<UniformBufferAllocation> UniformBufferPoolAllocator::Allocate(VkDeviceSize minimum_size) {
  VR_ASSERT(minimum_size <= block_size_);

  CreateBufferInfo create_info{};
  create_info.buffer_size = minimum_size;
  create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  create_info.memory_usage = VMA_MEMORY_USAGE_CPU_ONLY;
  create_info.pool = pool_;
  auto buffer = render_core_.CreateBuffer(create_info);

  return std::make_shared<UniformBufferAllocation>(buffer, alignment_, block_size_);
}

}  // namespace vre::rendering