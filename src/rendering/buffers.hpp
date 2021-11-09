#pragma once

#include <vulkan/vulkan_core.h>
#include "common.hpp"

namespace vre {

class Application;

namespace rendering {

// TODO(dmitrygladky): destructors
class Buffer {
 public:
  Buffer(VkDevice device, VkBuffer buffer, VkDeviceMemory buffer_memory, VkDeviceSize size)
      : device(device), buffer(buffer), buffer_memory(buffer_memory), size(size) {}

  void Update(const void *data) {
    void *buffer_data = nullptr;
    vkMapMemory(device, buffer_memory, 0, size, 0, &buffer_data);
    memcpy(buffer_data, data, size);
    vkUnmapMemory(device, buffer_memory);
  }

  VkDevice device;
  VkDeviceSize size;
  VkBuffer buffer;
  VkDeviceMemory buffer_memory;
};

class IndexBuffer : public Buffer {
 public:
  using Buffer::Buffer;

  static constexpr VkBufferUsageFlagBits kBufferBit = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
};

class VertexBuffer : public Buffer {
 public:
  using Buffer::Buffer;

  static constexpr VkBufferUsageFlagBits kBufferBit = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
};

class UniformBuffer : public Buffer {
 public:
  using Buffer::Buffer;

  static constexpr VkBufferUsageFlagBits kBufferBit = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
};

struct Vertex {
  glm::vec3 pos_;

  static VkVertexInputBindingDescription GetBindingDescription();
  static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
};

}  // namespace rendering
}  // namespace vre