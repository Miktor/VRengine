#pragma once

#include "common.hpp"

namespace vre {

class Application;

namespace rendering {

// TODO(dmitrygladky): destructors
class Buffer {
 public:
  Buffer(VkBuffer buffer, VkDeviceMemory buffer_memory) : buffer(buffer), buffer_memory(buffer_memory) {}

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

struct Vertex {
  glm::vec3 pos_;

  static VkVertexInputBindingDescription GetBindingDescription();
  static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();
};

}  // namespace rendering
}  // namespace vre