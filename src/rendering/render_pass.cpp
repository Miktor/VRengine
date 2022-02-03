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

Framebuffer::~Framebuffer() { vkDestroyFramebuffer(device_, framebuffer_, nullptr); }

}  // namespace vre::rendering