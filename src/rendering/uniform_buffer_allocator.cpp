#include "uniform_buffer_allocator.hpp"

#include "rendering/render_core.hpp"

namespace vre::rendering {

UniformBufferPoolAllocator::UniformBufferPoolAllocator(RenderCore &render_core, VkDeviceSize block_size,
                                                       VkDeviceSize alignment, VkBufferUsageFlags usage)
    : render_core_(render_core), block_size_(block_size), alignment_(alignment), usage_(usage) {}

UniformBufferAllocation UniformBufferPoolAllocator::Allocate(VkDeviceSize minimum_size) {
  VR_ASSERT(minimum_size <= block_size_);

  VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  auto buffer = render_core_.CreateBuffer(usage, minimum_size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  UniformBufferAllocation allocation;

  allocation.buffer = buffer;
  allocation.offset = 0;
  allocation.alignment = alignment_;
  allocation.size = block_size_;

  return allocation;
}
}  // namespace vre::rendering