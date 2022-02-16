#include "render_pass.hpp"

#include <vulkan/vulkan_core.h>

namespace vre::rendering {

namespace {

void compute_dimensions(const RenderPassInfo &info, uint32_t &width, uint32_t &height) {
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

RenderPass::~RenderPass() {
  vkDestroyRenderPass(device_, render_pass_, nullptr);
}

Framebuffer::Framebuffer(VkDevice device, RenderPass &render_pass, const RenderPassInfo &info)
    : device_(device), render_pass_(render_pass), info_(info) {
  compute_dimensions(info_, width_, height_);

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