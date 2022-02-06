#pragma once

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
  RenderPass(VkDevice device, const RenderPassInfo &info) : device_(device) {
    std::vector<VkAttachmentDescription> vk_color_attachments;
    vk_color_attachments.reserve(info.color_attachments.size());

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

      auto &image = color_attachment->GetImage();

      VkAttachmentDescription vk_attachment{};
      vk_attachment.format = color_attachment->GetFormat();
      vk_attachment.samples = image.GetCreateInfo().samples;
      vk_attachment.loadOp = get_load_op(i);
      vk_attachment.storeOp = get_store_op(i);
      vk_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      vk_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      vk_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      vk_attachment.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;

      if (color_attachment->GetImage().IsSwapchainImage()) {
        if (vk_attachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD)
          vk_attachment.initialLayout = image.GetSwapchainLayout();
        else
          vk_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        vk_attachment.finalLayout = image.GetSwapchainLayout();
      }

      vk_color_attachments.push_back(std::move(vk_attachment));
    }

    std::vector<VkSubpassDescription> vk_subpasses;
    std::list<std::vector<VkAttachmentReference>> vk_color_attachment_references;
    for (int i = 0; i < info.subpasses.size(); ++i) {
      vk_color_attachment_references.push_back({});
      auto &colors = vk_color_attachment_references.back();
      for (const auto &color_attachment : info.subpasses[i].color_attachments) {
        VkAttachmentReference vk_color_attachment_ref{};
        vk_color_attachment_ref.attachment = color_attachment;
        vk_color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colors.push_back(std::move(vk_color_attachment_ref));
      }

      VkSubpassDescription vk_subpass{};
      vk_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      vk_subpass.colorAttachmentCount = colors.size();
      vk_subpass.pColorAttachments = colors.data();
      vk_subpasses.push_back(std::move(vk_subpass));
    }

    // TODO: Resolve dependecies
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = vk_color_attachments.size();
    render_pass_info.pAttachments = vk_color_attachments.data();
    render_pass_info.subpassCount = vk_subpasses.size();
    render_pass_info.pSubpasses = vk_subpasses.data();

    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    CHECK_VK_SUCCESS(vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_));
  }

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