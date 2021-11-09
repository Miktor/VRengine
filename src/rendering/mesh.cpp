#include "mesh.hpp"
#include <vulkan/vulkan_core.h>

#include "application.hpp"
#include "rendering/render_core.hpp"

namespace vre::rendering {

void Mesh::AddPrimitive(std::vector<glm::vec3> vert, const std::vector<uint32_t> &indicies) {
  const uint32_t index_start = static_cast<uint32_t>(indicies_.size());
  const uint32_t vertex_start = static_cast<uint32_t>(pos_.size());

  pos_.insert(pos_.end(), std::make_move_iterator(vert.begin()), std::make_move_iterator(vert.end()));

  indicies_.reserve(indicies_.size() + indicies.size());
  for (const auto &index : indicies) {
    indicies_.push_back(index + vertex_start);
  }

  Primitive primitive{};
  primitive.index_count_ = indicies.size();
  primitive.index_start_ = index_start;
  primitive.vertex_count_ = vert.size();
  primitive.vertex_start_ = vertex_start;
  primitives_.push_back(primitive);
}

void Mesh::InitializeVulkan(RenderCore &renderer) {
  if (!pos_.empty()) {
    vertex_buffer_ = renderer.CreateVertexBuffer(pos_);
  }

  if (!indicies_.empty()) {
    index_buffer_ = renderer.CreateIndexBuffer(indicies_);
  }
}

void Mesh::Render(VkCommandBuffer command_buffers, VkPipelineLayout pipeline_layout, VkDescriptorSet descriptor_set) {
  VkBuffer vertex_buffers[] = {vertex_buffer_->buffer};
  VkDeviceSize offsets[] = {0};

  vkCmdBindVertexBuffers(command_buffers, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(command_buffers, index_buffer_->buffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(command_buffers, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

  // TODO(dmitrygladky): normal primitive rendering
  for (const auto &primitive : primitives_) {
    vkCmdDrawIndexed(command_buffers, static_cast<uint32_t>(primitive.index_count_), 1, 0, 0, 0);
  }
}

}  // namespace vre::rendering