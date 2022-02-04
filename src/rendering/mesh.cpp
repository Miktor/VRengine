#include "mesh.hpp"
#include <vulkan/vulkan_core.h>

#include "application.hpp"
#include "rendering/render_core.hpp"
#include "rendering/shader.hpp"

namespace vre::rendering {

std::shared_ptr<Material> GetDefaultMaterial(VkDevice device) {
  std::shared_ptr<Material> kMaterial;
  if (!kMaterial) {
    kMaterial = std::make_shared<Material>(device, Shader(device, Shader::kFragment, "assets/shaders/shader.frag"),
                                           Shader(device, Shader::kVertex, "assets/shaders/shader.vert"));
  }
  return kMaterial;
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
  primitive.index_count_ = indicies.size();
  primitive.index_start_ = index_start;
  primitive.vertex_count_ = vert.size();
  primitive.vertex_start_ = vertex_start;
  primitives_.push_back(std::move(primitive));
}

void Mesh::InitializeVulkan(RenderCore &renderer) {
  if (!pos_.empty()) {
    vertex_buffer_ = renderer.CreateVertexBuffer(pos_);
  }

  if (!indicies_.empty()) {
    index_buffer_ = renderer.CreateIndexBuffer(indicies_);
  }

  material_ = GetDefaultMaterial(renderer.GetDevice());
}

void Mesh::Render(rendering::RenderContext &context) {
  context.command_buffer.BindVertexBuffers(0, *vertex_buffer_, 0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX);
  context.command_buffer.BindIndexBuffer(*index_buffer_, 0, VK_INDEX_TYPE_UINT32);
  context.command_buffer.BindMaterial(*material_);

  vkCmdBindDescriptorSets(context.command_buffer.GetBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, material_->GetPipelineLayout(), 0, 1,
                          &context.descriptor_set, 0, nullptr);

  // TODO: make it for each mesh
  {
    rendering::UniformBufferObject data{};
    data.model = glm::mat4(1.0F);
    data.view = context.render_data.camera_view;
    data.proj = context.render_data.camera_projection;
    context.uniform_buffer->Update(&data);
  }

  context.command_buffer.BindUniformBuffer(0, 0, *context.uniform_buffer);

  // TODO(dmitrygladky): normal primitive rendering
  for (const auto &primitive : primitives_) {
    context.command_buffer.DrawIndexed(static_cast<uint32_t>(primitive.index_count_), 1, 0, 0, 0);
  }
}

}  // namespace vre::rendering