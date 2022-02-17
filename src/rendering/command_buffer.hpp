#pragma once

#include <vulkan/vulkan_core.h>
#include <memory>
#include <unordered_map>

#include "common.hpp"
#include "rendering/buffers.hpp"
#include "rendering/render_pass.hpp"
#include "rendering/shader.hpp"
#include "rendering/uniform_buffer_allocator.hpp"

namespace vre::rendering {

class RenderCore;

struct BeginRenderInfo {
  RenderPassInfo render_pass_info;
  std::shared_ptr<RenderPass> render_pass;
  std::shared_ptr<Framebuffer> framebuffer;
};

struct GraphicsState {
  struct {
    void Reset() {
      is_wireframe = false;
      render_pass.reset();
    }

    bool is_wireframe = false;
    std::shared_ptr<RenderPass> render_pass;
  } transient;

  struct {
    void Reset() {
      descriptor_sets.clear();
      material = nullptr;
      resource_bindings.clear();
    }

    std::unordered_map<uint32_t, VkDescriptorSet> descriptor_sets;
    Material *material = nullptr;
    ResourceBindings resource_bindings;
  } per_draw;

  void Reset() {
    transient.Reset();
    per_draw.Reset();
  }
};

class CommandBuffer {
 public:
  CommandBuffer(RenderCore *core, VkCommandBuffer command_buffer, VkPipelineCache cache)
      : core_(core), command_buffer_(command_buffer), cache_(cache) {}

  ~CommandBuffer();

  CommandBuffer(CommandBuffer &) = delete;
  CommandBuffer(CommandBuffer &&) = delete;

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
  void BindUniformBuffer(uint32_t set, uint32_t binding, const Buffer &buffer, const VkDeviceSize offset,
                         const VkDeviceSize size);

  void AllocateUniformBuffer(uint32_t set, uint32_t binding, const VkDeviceSize size, const void *data);

  void BindMaterial(Material &material);

  void DrawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset,
                   uint32_t first_instance);

 private:
  RenderCore *core_ = nullptr;
  VkCommandBuffer command_buffer_;
  VkPipelineCache cache_;

  GraphicsState state_;
  std::shared_ptr<UniformBufferAllocation> ubo_allocated_data_;

 private:
  void FlushState();
  void BindDescriptorSet(uint32_t set);
  VkPipeline GetGraphicsPipeline();
  VkPipeline BuildGraphicsPipeline();
};
}  // namespace vre::rendering