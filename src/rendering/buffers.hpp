#pragma once

#include "common.hpp"

namespace vre {

class Application;

namespace rendering {

// TODO(dmitrygladky): destructors
class Buffer {
 public:
  Buffer(VkDevice device, VkBuffer buffer, VkDeviceMemory buffer_memory, VkDeviceSize size)
      : device_(device), buffer_(buffer), buffer_memory_(buffer_memory), size_(size) {}

  void Update(const void *data) {
    void *buffer_data = nullptr;
    vkMapMemory(device_, buffer_memory_, 0, size_, 0, &buffer_data);
    memcpy(buffer_data, data, size_);
    vkUnmapMemory(device_, buffer_memory_);
  }

  [[nodiscard]] VkDeviceSize GetSize() const { return size_; }
  [[nodiscard]] VkBuffer GetBuffer() const { return buffer_; }

 private:
  VkDevice device_;
  VkDeviceSize size_;
  VkBuffer buffer_;
  VkDeviceMemory buffer_memory_;
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