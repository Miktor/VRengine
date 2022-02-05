#pragma once

#include <vulkan/vulkan_core.h>
#include <memory>
#include <unordered_map>

#include "common.hpp"
#include "rendering/buffers.hpp"
#include "rendering/pipeline.hpp"
#include "rendering/render_pass.hpp"
#include "rendering/shader.hpp"

namespace vre::rendering {

class RenderCore;

struct BeginRenderInfo {
  RenderPassInfo render_pass_info;
  std::shared_ptr<RenderPass> render_pass;
  std::shared_ptr<Framebuffer> framebuffer;
};

struct ResourceBinding {
  VkBuffer buffer;
};

using SetResourceBindings = std::unordered_map<uint8_t, ResourceBinding>;
using ResourceBindings = std::unordered_map<uint8_t, SetResourceBindings>;

struct GraphicsState {
  bool is_wireframe = false;

  std::shared_ptr<RenderPass> render_pass;

  std::unordered_map<uint32_t, VkDescriptorSet> descriptor_sets;
  const Material *material = nullptr;

  ResourceBindings resource_bindings_;
};

class CommandBuffer {
 public:
  CommandBuffer(RenderCore *core, VkCommandBuffer command_buffer)
      : core_(core), command_buffer_(command_buffer) {}

  CommandBuffer(CommandBuffer &) = delete;
  CommandBuffer(CommandBuffer &&) = default;

  // TODO: delete
  VkCommandBuffer GetBuffer() { return command_buffer_; }

  void Start();

  void BeginRenderPass(const BeginRenderInfo &info);

  void SetViewport(const VkViewport &viewport);
  void SetScissors(const VkRect2D &scissor);

  void SetDescriptorSet(uint8_t set, VkDescriptorSet descriptor_set);

  void BindVertexBuffers(uint32_t binding, const Buffer &buffer, VkDeviceSize offset, VkDeviceSize stride,
                         VkVertexInputRate step_rate);
  void BindIndexBuffer(const Buffer &buffer, VkDeviceSize offset, VkIndexType index_type);
  void BindUniformBuffer(uint32_t set, uint32_t binding, const Buffer &buffer);

  void BindMaterial(const Material &material);

  void DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset,
                   uint32_t first_instance);

 private:
  RenderCore *core_;
  VkCommandBuffer command_buffer_;

  GraphicsState state_;

 private:
  void BindDescriptorSet(uint32_t set);
  VkPipeline BuildGraphicsPipeline();
};
}  // namespace vre::rendering