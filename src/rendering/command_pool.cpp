#include "command_pool.hpp"

namespace vre::scene {

CommandPool::CommandPool(VkCommandPool command_pool) : command_pool_(command_pool) {}

VkCommandBuffer CommandPool::GetCommandBuffer() {
  if (ready_buffers_.empty()) {
    CreateCommandBuffer();
  }

  const auto buffer = *ready_buffers_.begin();
  ready_buffers_.erase(ready_buffers_.begin());
  in_use_buffers_.insert(buffer);
  return buffer;
}

VkCommandBuffer CommandPool::CreateCommandBuffer() {
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer buffer;
  if (vkAllocateCommandBuffers(device_, &alloc_info, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  ready_buffers_.insert(buffer);

  return buffer;
}
}  // namespace vre::scene