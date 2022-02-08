#include "uniform_buffer_allocator.hpp"

#include "helpers.hpp"
#include "rendering/render_core.hpp"
#include "vk_mem_alloc.h"

namespace vre::rendering {

UniformBufferPoolAllocator::UniformBufferPoolAllocator(RenderCore &render_core, VkDeviceSize block_size,
                                                       VkDeviceSize alignment, VkBufferUsageFlags usage)
    : render_core_(render_core), block_size_(block_size), alignment_(alignment), usage_(usage) {
  VmaPoolCreateInfo pool_create_info;
  pool_create_info.blockSize = block_size_;
  pool_create_info.minBlockCount = 2;
  pool_create_info.minAllocationAlignment = alignment_;

  CHECK_VK_SUCCESS(vmaCreatePool(render_core_.GetVmaAllocator(), &pool_create_info, &pool_));
}

UniformBufferPoolAllocator::~UniformBufferPoolAllocator() {
  vmaDestroyPool(render_core_.GetVmaAllocator(), pool_);
}

UniformBufferAllocation UniformBufferPoolAllocator::Allocate(VkDeviceSize minimum_size) {
  VR_ASSERT(minimum_size <= block_size_);

  CreateBufferInfo create_info{};
  create_info.buffer_size = minimum_size;
  create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  create_info.memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
  create_info.pool = pool_;
  auto buffer = render_core_.CreateBuffer(create_info);

  UniformBufferAllocation allocation;

  allocation.buffer = buffer;
  allocation.offset = 0;
  allocation.alignment = alignment_;
  allocation.size = block_size_;

  return allocation;
}
}  // namespace vre::rendering