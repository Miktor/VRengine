#include "uniform_buffer_allocator.hpp"

#include "rendering/render_core.hpp"
#include "vk_mem_alloc.h"

namespace vre::rendering {

UniformBufferPoolAllocator::UniformBufferPoolAllocator(RenderCore &render_core, VkDeviceSize block_size,
                                                       VkDeviceSize alignment, VkBufferUsageFlags usage)
    : render_core_(render_core), block_size_(block_size), alignment_(alignment), usage_(usage) {}

UniformBufferAllocation UniformBufferPoolAllocator::Allocate(VkDeviceSize minimum_size) {
  VR_ASSERT(minimum_size <= block_size_);

  VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  auto buffer = render_core_.CreateBuffer(minimum_size, usage, VMA_MEMORY_USAGE_GPU_ONLY);

  UniformBufferAllocation allocation;

  allocation.buffer = buffer;
  allocation.offset = 0;
  allocation.alignment = alignment_;
  allocation.size = block_size_;

  return allocation;
}
}  // namespace vre::rendering