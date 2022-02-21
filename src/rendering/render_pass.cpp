#include "render_pass.hpp"

#include <vulkan/vulkan_core.h>
#include <vector>

namespace vre::rendering {

namespace {

void ComputeDimensions(const RenderPassInfo &info, uint32_t &width, uint32_t &height) {
  width = UINT32_MAX;
  height = UINT32_MAX;
  VR_ASSERT(!info.color_attachments.empty());

  for (const auto &attachment : info.color_attachments) {
    width = std::min(width, attachment->GetImage().GetWidth());
    height = std::min(height, attachment->GetImage().GetHeight());
  }
}

std::vector<VkImageView> GetViews(const RenderPassInfo &info) {
  std::vector<VkImageView> views;
  views.reserve(info.color_attachments.size());

  for (const auto &attachment : info.color_attachments) {
    views.push_back(attachment->GetRenderTargetView());
  }

  return views;
}

}  // namespace

RenderPass::RenderPass(VkDevice device, const RenderPassInfo &info) : device_(device) {
  const auto get_load_op = [&info](unsigned index) -> VkAttachmentLoadOp {
    if ((info.clear_attachments & (1u << index)) != 0) {
      return VK_ATTACHMENT_LOAD_OP_CLEAR;
    }

    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  };

  const auto get_store_op = [&info](unsigned index) -> VkAttachmentStoreOp {
    if ((info.store_attachments & (1u << index)) != 0) {
      return VK_ATTACHMENT_STORE_OP_STORE;
    }

    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
  };

  std::vector<VkAttachmentDescription> vk_attachments;
  vk_attachments.reserve(info.color_attachments.size());
  for (const auto &[i, color_attachment] : Enumerate(info.color_attachments)) {
    auto &image = color_attachment->GetImage();

    auto &vk_attachment = vk_attachments.emplace_back();
    vk_attachment.format = color_attachment->GetFormat();
    vk_attachment.samples = image.GetCreateInfo().samples;
    vk_attachment.loadOp = get_load_op(i);
    vk_attachment.storeOp = get_store_op(i);
    vk_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vk_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vk_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vk_attachment.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (color_attachment->GetImage().IsSwapchainImage()) {
      if (vk_attachment.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD) {
        vk_attachment.initialLayout = image.GetSwapchainLayout();
      } else {
        vk_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      }

      vk_attachment.finalLayout = image.GetSwapchainLayout();
    }
  }

  if (info.depth_stencil_attachment) {
    auto &ds_attachment = vk_attachments.emplace_back();
    ds_attachment.flags = 0;
    ds_attachment.format = info.depth_stencil_attachment->GetFormat();
    ds_attachment.samples = info.depth_stencil_attachment->GetImage().GetCreateInfo().samples;
    ds_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ds_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ds_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ds_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ds_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ds_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  }

  std::vector<VkSubpassDescription> vk_subpasses;
  std::list<std::vector<VkAttachmentReference>> vk_attachment_references;
  std::vector<VkSubpassDependency> vk_dependencies;
  for (const auto &[i, subpass] : Enumerate(info.subpasses)) {
    auto &vk_attachment = vk_attachment_references.emplace_back();
    for (const auto &color_attachment : subpass.color_attachments) {
      auto &vk_color_attachment_ref = vk_attachment.emplace_back();
      vk_color_attachment_ref.attachment = color_attachment;
      vk_color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    const uint32_t color_attachment_count = vk_attachment.size();

    VkAttachmentReference *depth_stencil_attachment = nullptr;
    if (subpass.use_depth_stencil) {
      VR_ASSERT(info.depth_stencil_attachment);

      auto &vk_depth_attachment_ref = vk_attachment.emplace_back();
      vk_depth_attachment_ref.attachment = vk_attachments.size() - 1;  // DS always last
      vk_depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      depth_stencil_attachment = &vk_depth_attachment_ref;
    }

    auto &vk_subpass = vk_subpasses.emplace_back();
    vk_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    vk_subpass.colorAttachmentCount = color_attachment_count;
    vk_subpass.pColorAttachments = vk_attachment.data();
    vk_subpass.pDepthStencilAttachment = depth_stencil_attachment;

    auto &dependency = vk_dependencies.emplace_back();
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = i;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    if (subpass.use_depth_stencil) {
      dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
      dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
  }

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = vk_attachments.size();
  render_pass_info.pAttachments = vk_attachments.data();
  render_pass_info.subpassCount = vk_subpasses.size();
  render_pass_info.pSubpasses = vk_subpasses.data();
  render_pass_info.dependencyCount = vk_dependencies.size();
  render_pass_info.pDependencies = vk_dependencies.data();

  CHECK_VK_SUCCESS(vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_));
}

RenderPass::~RenderPass() {
  vkDestroyRenderPass(device_, render_pass_, nullptr);
}

Framebuffer::Framebuffer(VkDevice device, RenderPass &render_pass, const RenderPassInfo &info)
    : device_(device), render_pass_(render_pass), info_(info) {
  ComputeDimensions(info_, width_, height_);

  auto views = GetViews(info_);

  VkFramebufferCreateInfo framebuffer_info{};
  framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_info.renderPass = render_pass_.GetRenderPass();
  framebuffer_info.attachmentCount = views.size();
  framebuffer_info.pAttachments = views.data();
  framebuffer_info.width = width_;
  framebuffer_info.height = height_;
  framebuffer_info.layers = 1;

  CHECK_VK_SUCCESS(vkCreateFramebuffer(device_, &framebuffer_info, nullptr, &framebuffer_));
}

Framebuffer::~Framebuffer() {
  vkDestroyFramebuffer(device_, framebuffer_, nullptr);
}

}  // namespace vre::rendering