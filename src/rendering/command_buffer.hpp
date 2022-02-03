#include <memory>
#include "common.hpp"

#include "rendering/buffers.hpp"
#include "rendering/pipeline.hpp"
#include "rendering/render_pass.hpp"

namespace vre::rendering {

class RenderCore;

struct BeginRenderInfo {
  RenderPassInfo render_pass_info;
  std::shared_ptr<RenderPass> render_pass;
  std::shared_ptr<Framebuffer> framebuffer;
  std::shared_ptr<Pipeline> pipeline;
};

class CommandBuffer {
 public:
  CommandBuffer(RenderCore *core, VkCommandBuffer command_buffer) : core_(core), command_buffer_(command_buffer) {}

  CommandBuffer(CommandBuffer &) = delete;
  CommandBuffer(CommandBuffer &&) = default;

  // TODO: delete
  VkCommandBuffer GetBuffer() { return command_buffer_; }

  void Start();

  void BeginRenderPass(const BeginRenderInfo &info);

  void BindVertexBuffers(uint32_t binding, const Buffer &buffer, VkDeviceSize offset, VkDeviceSize stride, VkVertexInputRate step_rate);
  void BindIndexBuffer(const Buffer &buffer, VkDeviceSize offset, VkIndexType index_type);

  void BindUniformBuffer(uint32_t set, uint32_t binding, const Buffer &buffer);

  void DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);

 private:
  const RenderCore *core_;
  VkCommandBuffer command_buffer_;
};
}  // namespace vre::rendering