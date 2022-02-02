#include "command_buffer.hpp"

namespace vre::rendering {

namespace {

VkRect2D GetScissors(const BeginRenderInfo &info) {
  auto scissors = info.render_pass_info.render_area;

  uint32_t fb_width = info.framebuffer->GetWidth();
  uint32_t fb_height = info.framebuffer->GetHeight();

  scissors.offset.x = std::min(fb_width, uint32_t(scissors.offset.x));
  scissors.offset.y = std::min(fb_height, uint32_t(scissors.offset.y));
  scissors.extent.width = std::min(fb_width - scissors.offset.x, scissors.extent.width);
  scissors.extent.height = std::min(fb_height - scissors.offset.y, scissors.extent.height);

  return scissors;
}
}  // namespace

void CommandBuffer::Start() {
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(command_buffer_, &begin_info) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }
}

void CommandBuffer::BeginRenderPass(const BeginRenderInfo &info) {
  std::vector<VkClearValue> clear_values;
  clear_values.resize(info.render_pass_info.color_attachments.size());
  uint32_t num_clear_values = 0;

  for (unsigned i = 0; i < info.render_pass_info.color_attachments.size(); i++) {
    const auto &clear_color = info.render_pass_info.clear_color[i];

    if (info.render_pass_info.clear_attachments & (1u << i)) {
      clear_values[i].color = clear_color;
      num_clear_values = i + 1;
    }
  }
  clear_values.resize(num_clear_values);

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = info.render_pass->GetRenderPass();
  render_pass_info.framebuffer = info.framebuffer->GetFramebuffer();
  render_pass_info.renderArea = GetScissors(info);
  render_pass_info.clearValueCount = clear_values.size();
  render_pass_info.pClearValues = clear_values.data();

  vkCmdBeginRenderPass(command_buffer_, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline->GetPipeline());
}

}  // namespace vre::rendering