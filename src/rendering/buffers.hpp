#pragma once

#include "common.hpp"

namespace vre {

class Application;

namespace rendering {

// TODO(dmitrygladky): destructors
class Buffer {
 public:
  Buffer(VkBuffer buffer, VmaAllocator vma_allocator, VmaAllocation vma_allocation, VkDeviceSize size)
      : buffer_(buffer), vma_allocator_(vma_allocator), vma_allocation_(vma_allocation), size_(size) {}

  // TODO: bind memory on creation if possible
  void Update(const void *data) {
    void *buffer_data = nullptr;
    CHECK_VK_SUCCESS(vmaMapMemory(vma_allocator_, vma_allocation_, &buffer_data));
    memcpy(buffer_data, data, size_);
    vmaUnmapMemory(vma_allocator_, vma_allocation_);
  }

  [[nodiscard]] VkDeviceSize GetSize() const { return size_; }
  [[nodiscard]] VkBuffer GetBuffer() const { return buffer_; }

 private:
  VkBuffer buffer_;
  VmaAllocator vma_allocator_;
  VmaAllocation vma_allocation_;
  VkDeviceSize size_;
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