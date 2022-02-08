#include "rendering/buffers.hpp"
#include <vulkan/vulkan_core.h>
#include <tuple>

#include "helpers.hpp"
#include "rendering/render_core.hpp"
#include "vk_mem_alloc.h"

namespace vre::rendering {

namespace {

void CopyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size, VkDevice device,
                VkCommandPool command_pool, VkQueue queue) {
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  VkBufferCopy copy_region{};
  copy_region.size = size;
  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

auto CreateBufferImpl(VkBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage,
                      VmaMemoryUsage memory_usage, VmaAllocator vma_allocator) {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = memory_usage;
  alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VmaAllocation allocation;
  VmaAllocationInfo allocation_info;
  CHECK_VK_SUCCESS(
      vmaCreateBuffer(vma_allocator, &buffer_info, &alloc_info, &buffer, &allocation, &allocation_info));

  return std::pair(allocation, allocation_info);
}

}  // namespace

std::shared_ptr<Buffer> RenderCore::CreateBuffer(const CreateBufferInfo &crate_info) {
  VkBuffer buffer;
  auto [allocation, allocation_info] = CreateBufferImpl(buffer, crate_info.buffer_size, crate_info.usage,
                                                        crate_info.memory_usage, vma_allocator_);

  if (crate_info.initial_data != nullptr) {
    if (allocation_info.pMappedData == nullptr) {
      VkBuffer staging_buffer;
      auto [staging_allocation, staging_allocation_info] =
          CreateBufferImpl(staging_buffer, crate_info.buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VMA_MEMORY_USAGE_CPU_ONLY, vma_allocator_);
      VR_CHECK(staging_allocation_info.pMappedData);
      memcpy(staging_allocation_info.pMappedData, crate_info.initial_data,
             static_cast<size_t>(crate_info.buffer_size));
      CopyBuffer(staging_buffer, buffer, crate_info.buffer_size, device_, command_pool_, graphics_queue_);

      vmaDestroyBuffer(vma_allocator_, staging_buffer, staging_allocation);
    } else {
      memcpy(allocation_info.pMappedData, crate_info.initial_data, crate_info.buffer_size);
    }
  }

  return std::make_shared<Buffer>(buffer, vma_allocator_, allocation, crate_info.buffer_size,
                                  allocation_info);
}

}  // namespace vre::rendering