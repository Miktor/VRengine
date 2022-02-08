#include "rendering/buffers.hpp"
#include <vulkan/vulkan_core.h>

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

uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties,
                        VkPhysicalDevice physical_device) {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    if (((type_filter & (1 << i)) != 0U) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

// TODO: delete
VmaAllocation CreateBuffer(VkBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage,
                           VmaMemoryUsage memory_usage, VmaAllocator vma_allocator) {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = memory_usage;

  VmaAllocation allocation;
  CHECK_VK_SUCCESS(vmaCreateBuffer(vma_allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr));

  return allocation;
}

template <typename T>
std::shared_ptr<T> CrateBufferThroughtStaging(const void *buffer_data, VkDeviceSize buffer_size,
                                              VkDevice device, VmaAllocator vma_allocator,
                                              VkCommandPool command_pool, VkQueue queue) {
  VkBuffer buffer;
  auto allocation = CreateBuffer(buffer, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | T::kBufferBit,
                                 VMA_MEMORY_USAGE_GPU_ONLY, vma_allocator);

  {
    VkBuffer staging_buffer;
    auto staging_allocation = CreateBuffer(staging_buffer, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VMA_MEMORY_USAGE_CPU_ONLY, vma_allocator);

    void *data;
    CHECK_VK_SUCCESS(vmaMapMemory(vma_allocator, staging_allocation, &data));
    memcpy(data, buffer_data, static_cast<size_t>(buffer_size));
    vmaUnmapMemory(vma_allocator, staging_allocation);
    CopyBuffer(staging_buffer, buffer, buffer_size, device, command_pool, queue);

    vmaDestroyBuffer(vma_allocator, staging_buffer, staging_allocation);
  }

  return std::make_shared<T>(buffer, vma_allocator, allocation, buffer_size);
}

}  // namespace

std::shared_ptr<Buffer> RenderCore::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                                 VmaMemoryUsage memory_usage) {
  VkBuffer buffer;
  auto allocation = ::vre::rendering::CreateBuffer(buffer, size, usage, memory_usage, vma_allocator_);

  return std::make_shared<Buffer>(buffer, vma_allocator_, allocation, size);
}

std::shared_ptr<rendering::IndexBuffer> RenderCore::CreateIndexBuffer(const std::vector<uint32_t> &indices) {
  return CrateBufferThroughtStaging<rendering::IndexBuffer>(reinterpret_cast<const void *>(indices.data()),
                                                            sizeof(indices[0]) * indices.size(), device_,
                                                            vma_allocator_, command_pool_, graphics_queue_);
}

std::shared_ptr<rendering::VertexBuffer> RenderCore::CreateVertexBuffer(
    const std::vector<glm::vec3> &vertexes) {
  return CrateBufferThroughtStaging<rendering::VertexBuffer>(
      reinterpret_cast<const void *>(vertexes.data()), vertexes.size() * sizeof(vertexes.front()), device_,
      vma_allocator_, command_pool_, graphics_queue_);
}

std::shared_ptr<UniformBuffer> RenderCore::CreateUniformBuffer(const VkDeviceSize size) {
  VkBuffer buffer;
  auto allocation = ::vre::rendering::CreateBuffer(buffer, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                   VMA_MEMORY_USAGE_CPU_ONLY, vma_allocator_);

  return std::make_shared<UniformBuffer>(buffer, vma_allocator_, allocation, size);
}

VkVertexInputBindingDescription Vertex::GetBindingDescription() {
  VkVertexInputBindingDescription binding_description{};

  binding_description.binding = 0;
  binding_description.stride = sizeof(Vertex);
  binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  return binding_description;
}

std::vector<VkVertexInputAttributeDescription> Vertex::GetAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attribute_descriptions(1);

  attribute_descriptions[0].binding = 0;
  attribute_descriptions[0].location = 0;
  attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attribute_descriptions[0].offset = offsetof(Vertex, pos_);

  return attribute_descriptions;
}

}  // namespace vre::rendering