#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "helpers.hpp"
#include "image.hpp"

namespace vre::rendering {

class Framebuffer;

enum class RenderPassOp : uint32_t {
  RENDER_PASS_OP_CLEAR_DEPTH_STENCIL_BIT = 1 << 0,
  RENDER_PASS_OP_LOAD_DEPTH_STENCIL_BIT = 1 << 1,
  RENDER_PASS_OP_STORE_DEPTH_STENCIL_BIT = 1 << 2,
  RENDER_PASS_OP_DEPTH_STENCIL_READ_ONLY_BIT = 1 << 3,
  RENDER_PASS_OP_ENABLE_TRANSIENT_STORE_BIT = 1 << 4,
  RENDER_PASS_OP_ENABLE_TRANSIENT_LOAD_BIT = 1 << 5
};

struct RenderPassInfo {
  std::vector<ImageViewPtr> color_attachments;

  RenderPassOp op_flags;

  uint32_t clear_attachments = 0;
  uint32_t store_attachments = 0;

  std::shared_ptr<Framebuffer> framebuffer;

  // Render area will be clipped to the actual framebuffer.
  VkRect2D render_area = {{0, 0}, {UINT32_MAX, UINT32_MAX}};

  std::vector<VkClearColorValue> clear_color;

  enum class DepthStencil {
    None,
    ReadOnly,
    ReadWrite,
  };

  struct Subpass {
    std::vector<uint32_t> color_attachments;
  };

  std::vector<Subpass> subpasses;
};

class RenderPass {
 public:
  RenderPass(RenderPass &) = delete;
  RenderPass(RenderPass &&) = delete;

  RenderPass(VkDevice device, const RenderPassInfo &info);

  ~RenderPass();

  VkRenderPass GetRenderPass() { return render_pass_; }

 private:
  VkDevice device_;
  VkRenderPass render_pass_;

 private:
};

class Framebuffer {
 public:
  Framebuffer(VkDevice device, RenderPass &render_pass, const RenderPassInfo &info);

  ~Framebuffer();

  VkFramebuffer GetFramebuffer() const { return framebuffer_; }

  uint32_t GetWidth() const { return width_; }

  uint32_t GetHeight() const { return height_; }

  const RenderPass &GetCompatibleRenderPass() const { return render_pass_; }

 private:
  VkDevice device_;
  VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
  RenderPass &render_pass_;
  RenderPassInfo info_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
};

}  // namespace vre::rendering