#include <memory>
#include "common.hpp"

#include "rendering/render_pass.hpp"
#include "rendering/pipeline.hpp"

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

 private:
  const RenderCore *core_;
  VkCommandBuffer command_buffer_;
};
}  // namespace vre::rendering