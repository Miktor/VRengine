#pragma once

#include "common.hpp"

#include "rendering/buffers.hpp"

namespace vre ::rendering {

class RenderCore;

struct Primitive {
  uint32_t index_start_ = 0;
  uint32_t vertex_start_ = 0;

  uint32_t index_count_ = 0;
  uint32_t vertex_count_ = 0;
};

class Mesh {
 public:
  void AddPrimitive(std::vector<glm::vec3> vert, const std::vector<uint32_t> &indicies);

  void InitializeVulkan(RenderCore &renderer);
  void Render(VkCommandBuffer command_buffers, VkPipelineLayout pipeline_layout, VkDescriptorSet descriptor_set);

 private:
  std::vector<Primitive> primitives_;

  std::vector<glm::vec3> pos_;
  std::vector<uint32_t> indicies_;

  std::shared_ptr<IndexBuffer> index_buffer_;
  std::shared_ptr<VertexBuffer> vertex_buffer_;
};

}  // namespace vre::rendering