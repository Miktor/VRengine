#include "common.hpp"

namespace vre::rendering {

class RenderCore;

class CommandBuffer {
 public:
  CommandBuffer(RenderCore *core, VkCommandBuffer command_buffer) : core_(core), command_buffer_(command_buffer) {}

  CommandBuffer(CommandBuffer &) = delete;
  CommandBuffer(CommandBuffer &&) = default;

  void Start() {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command_buffer_, &begin_info) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }
  }

  // void BeginRenderPass(const RenderPassInfo &info) {
  //   VkRenderPassBeginInfo render_pass_info{};
  //   render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  //   render_pass_info.renderPass = render_pass;
  //   render_pass_info.framebuffer = frame_buffer;
  //   render_pass_info.renderArea.offset = {0, 0};
  //   render_pass_info.renderArea.extent = swap_chain_extent;

  //   VkClearValue clear_color = {0.0F, 0.0F, 0.0F, 1.0F};
  //   render_pass_info.clearValueCount = 1;
  //   render_pass_info.pClearValues = &clear_color;

  //   vkCmdBeginRenderPass(command_buffer_, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  //   vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetPipeline());
  // }

 private:
  const RenderCore *core_;
  VkCommandBuffer command_buffer_;
};
}  // namespace vre::rendering