#include "mesh.hpp"
#include <vulkan/vulkan_core.h>

#include "application.hpp"
#include "rendering/render_core.hpp"
#include "rendering/shader.hpp"

namespace vre::rendering {

std::shared_ptr<Material> GetDefaultMaterial(VkDevice device) {
  return std::make_shared<Material>(
      device, std::make_shared<Shader>(device, Shader::kFragment, "assets/shaders/shader.frag"),
      std::make_shared<Shader>(device, Shader::kVertex, "assets/shaders/shader.vert"));
}

void Mesh::AddPrimitive(std::vector<glm::vec3> vert, const std::vector<uint32_t> &indicies) {
  const uint32_t index_start = static_cast<uint32_t>(indicies_.size());
  const uint32_t vertex_start = static_cast<uint32_t>(pos_.size());

  pos_.insert(pos_.end(), std::make_move_iterator(vert.begin()), std::make_move_iterator(vert.end()));

  indicies_.reserve(indicies_.size() + indicies.size());
  for (const auto &index : indicies) {
    indicies_.push_back(index + vertex_start);
  }

  Primitive primitive{};
  primitive.index_count = indicies.size();
  primitive.index_start = index_start;
  primitive.vertex_count = vert.size();
  primitive.vertex_start = vertex_start;
  primitives_.push_back(primitive);
}

void Mesh::InitializeVulkan(RenderCore &renderer) {
  if (!pos_.empty()) {
    CreateBufferInfo create_info{};
    create_info.buffer_size = pos_.size() * sizeof(glm::vec3);
    create_info.initial_data = pos_.data();
    create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    create_info.memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vertex_buffer_ = renderer.CreateBuffer(create_info);
  }

  if (!indicies_.empty()) {
    CreateBufferInfo create_info{};
    create_info.buffer_size = indicies_.size() * sizeof(uint32_t);
    create_info.initial_data = indicies_.data();
    create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    create_info.memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;

    index_buffer_ = renderer.CreateBuffer(create_info);
  }

  material_ = GetDefaultMaterial(renderer.GetDevice());
}

void Mesh::Render(rendering::RenderContext &context, const glm::mat4 &transform) {
  context.command_buffer->BindMaterial(*material_);
  context.command_buffer->BindVertexBuffers(0, *vertex_buffer_, 0, sizeof(glm::vec3),
                                            VK_VERTEX_INPUT_RATE_VERTEX);
  context.command_buffer->BindIndexBuffer(*index_buffer_, 0, VK_INDEX_TYPE_UINT32);

  {
    rendering::UniformBufferObject data{};
    data.model = transform;
    data.view = context.render_data.camera_view;
    data.proj = context.render_data.camera_projection;

    context.command_buffer->AllocateUniformBuffer(0, 0, data);
  }

  // TODO(dmitrygladky): normal primitive rendering
  for (const auto &primitive : primitives_) {
    context.command_buffer->DrawIndexed(static_cast<uint32_t>(primitive.index_count), 1, 0, 0, 0);
  }
}

}  // namespace vre::rendering