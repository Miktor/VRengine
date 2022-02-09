#pragma once

#include "common.hpp"

#include "rendering/buffers.hpp"
#include "vk_mem_alloc.h"

namespace vre::rendering {

class RenderCore;

struct UBOAllocation {
  VkDeviceSize offset;
  VkDeviceSize size;
};

class UniformBufferAllocation {
 public:
  UniformBufferAllocation(std::shared_ptr<Buffer> buffer, VkDeviceSize alignment, VkDeviceSize size);

  UniformBufferAllocation(UniformBufferAllocation &) = delete;
  UniformBufferAllocation(UniformBufferAllocation &&) = default;

  [[nodiscard]] const Buffer &GetBuffer() const;
  [[nodiscard]] UBOAllocation Allocate(VkDeviceSize size);

 private:
  std::shared_ptr<Buffer> buffer_;
  VkDeviceSize offset_ = 0;
  const VkDeviceSize alignment_ = 0;
  const VkDeviceSize size_ = 0;
};

// TODO(dmitrygladky): add actual pool functionality
class UniformBufferPoolAllocator {
 public:
  UniformBufferPoolAllocator(RenderCore &render_core, VkDeviceSize block_size, VkDeviceSize alignment,
                             VkBufferUsageFlags usage);
  ~UniformBufferPoolAllocator();

  std::shared_ptr<UniformBufferAllocation> Allocate(VkDeviceSize minimum_size);

 private:
  RenderCore &render_core_;

  VmaPool pool_;

  VkDeviceSize block_size_;
  VkDeviceSize alignment_;
  VkBufferUsageFlags usage_;
};

}  // namespace vre::rendering