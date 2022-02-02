#include "image.hpp"

namespace vre::rendering {

ImageView::ImageView(VkImageView view, ImageViewCreateInfo &&info) : view_(view), info_(std::move(info)) {}

Image::Image(VkDevice device, VkImage image, VkImageView default_view, const ImageCreateInfo &info, VkImageViewType view_type)
    : info_(info) {
  ImageViewCreateInfo view_info;
  view_info.image = this;
  view_info.view_type = view_type;
  view_info.format = info.format;
  view_info.base_level = 0;
  view_info.levels = info.levels;
  view_info.base_layer = 0;
  view_info.layers = info.layers;
  view_ = std::make_shared<ImageView>(default_view, std::move(view_info));
}

}  // namespace vre::rendering