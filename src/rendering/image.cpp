#include "image.hpp"
#include <vulkan/vulkan_core.h>

namespace vre::rendering {

ImageView::ImageView(VkDevice device, VkImageView view, ImageViewCreateInfo &&info)
    : device_(device), view_(view), info_(std::move(info)) {}

ImageView::~ImageView() { vkDestroyImageView(device_, view_, nullptr); }

Image::Image(VkDevice device, VkImage image, VkImageView default_view, const ImageCreateInfo &info,
             VkImageViewType view_type)
    : device_(device), info_(info) {
  ImageViewCreateInfo view_info;
  view_info.image = this;
  view_info.view_type = view_type;
  view_info.format = info.format;
  view_info.base_level = 0;
  view_info.levels = info.levels;
  view_info.base_layer = 0;
  view_info.layers = info.layers;
  view_ = std::make_shared<ImageView>(device_, default_view, std::move(view_info));
}

}  // namespace vre::rendering