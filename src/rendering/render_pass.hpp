#pragma once

#include "common.hpp"
#include "image.hpp"

namespace vre::rendering {

enum RenderPassOp {
  RENDER_PASS_OP_CLEAR_DEPTH_STENCIL_BIT = 1 << 0,
  RENDER_PASS_OP_LOAD_DEPTH_STENCIL_BIT = 1 << 1,
  RENDER_PASS_OP_STORE_DEPTH_STENCIL_BIT = 1 << 2,
  RENDER_PASS_OP_DEPTH_STENCIL_READ_ONLY_BIT = 1 << 3,
  RENDER_PASS_OP_ENABLE_TRANSIENT_STORE_BIT = 1 << 4,
  RENDER_PASS_OP_ENABLE_TRANSIENT_LOAD_BIT = 1 << 5
};

struct RenderPassInfo {
  std::vector<ImageView> color_attachments;

  RenderPassOp op_flags;

  uint32_t clear_attachments = 0;
  uint32_t load_attachments = 0;
  uint32_t store_attachments = 0;

  // Render area will be clipped to the actual framebuffer.
  VkRect2D render_area = {{0, 0}, {UINT32_MAX, UINT32_MAX}};

  std::vector<VkClearColorValue> clear_color;
  VkClearDepthStencilValue clear_depth_stencil{1.0f, 0};

  enum class DepthStencil {
    None,
    ReadOnly,
    ReadWrite,
  };
};

class RenderPass {
 public:
  RenderPass(VkDevice device, const RenderPassInfo &info) : device_(device) {
    std::vector<VkAttachmentDescription> color_attachments;
    color_attachments.reserve(info.color_attachments.size());

    const auto get_load_op = [&info](unsigned index) -> VkAttachmentLoadOp {
      if ((info.clear_attachments & (1u << index)) != 0)
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
      else
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    };

    const auto get_store_op = [&info](unsigned index) -> VkAttachmentStoreOp {
      if ((info.store_attachments & (1u << index)) != 0)
        return VK_ATTACHMENT_STORE_OP_STORE;
      else
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    };

    for (int i = 0; i < info.color_attachments.size(); ++i) {
      const auto &color_attachment = info.color_attachments[i];

      auto &image = color_attachment.GetImage();

      VkAttachmentDescription vk_attachment{};
      vk_attachment.format = color_attachment.GetFormat();
      vk_attachment.samples = image.GetCreateInfo().samples;
      vk_attachment.loadOp = get_load_op(i);
      vk_attachment.storeOp = get_store_op(i);
      vk_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      vk_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      vk_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      vk_attachment.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;

      if (color_attachment.GetImage().IsSwapchainImage()) {
        if (vk_attachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
          vk_attachment.initialLayout = image.GetSwapchainLayout();
        else
          vk_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        vk_attachment.finalLayout = image.GetSwapchainLayout();
      }
    }
  }

 private:
  VkDevice device_;

 private:
};

}  // namespace vre::rendering