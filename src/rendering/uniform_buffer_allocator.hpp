#pragma once

#include "common.hpp"

#include "rendering/buffers.hpp"
#include "vk_mem_alloc.h"

namespace vre::rendering {

class RenderCore;

struct UniformBufferAllocation {
  std::shared_ptr<Buffer> buffer;
  VkDeviceSize offset;
  VkDeviceSize alignment;
  VkDeviceSize size;
};

// TODO(dmitrygladky): add actual pool functionality
class UniformBufferPoolAllocator {
 public:
  UniformBufferPoolAllocator(RenderCore &render_core, VkDeviceSize block_size, VkDeviceSize alignment,
                             VkBufferUsageFlags usage);
  ~UniformBufferPoolAllocator();
  
  UniformBufferAllocation Allocate(VkDeviceSize minimum_size);

 private:
  RenderCore &render_core_;

  VmaPool pool_;

  VkDeviceSize block_size_;
  VkDeviceSize alignment_;
  VkBufferUsageFlags usage_;
};

}  // namespace vre::rendering