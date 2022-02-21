#pragma once

#include <vulkan/vulkan_core.h>
#include "common.hpp"

namespace vre::rendering {

class Image;

struct ImageViewCreateInfo {
  Image *image = nullptr;
  VkFormat format = VK_FORMAT_UNDEFINED;

  unsigned base_level = 0;
  unsigned levels = VK_REMAINING_MIP_LEVELS;
  unsigned base_layer = 0;
  unsigned layers = VK_REMAINING_ARRAY_LAYERS;

  VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;

  VkComponentMapping swizzle = {
      VK_COMPONENT_SWIZZLE_R,
      VK_COMPONENT_SWIZZLE_G,
      VK_COMPONENT_SWIZZLE_B,
      VK_COMPONENT_SWIZZLE_A,
  };
};

class ImageView {
 public:
  ImageView(VkDevice device, VkImageView view, ImageViewCreateInfo &&info);
  ~ImageView();

  ImageView(ImageView &) = delete;
  ImageView(ImageView &&) = delete;

  [[nodiscard]] VkFormat GetFormat() const { return info_.format; }

  [[nodiscard]] const Image &GetImage() const { return *info_.image; }

  [[nodiscard]] const ImageViewCreateInfo &GetCreateInfo() const { return info_; }

  [[nodiscard]] VkImageView GetRenderTargetView() const { return view_; }

 private:
  VkDevice device_;
  VkImageView view_;
  const ImageViewCreateInfo info_;
};
using ImageViewPtr = std::shared_ptr<ImageView>;

struct ImageCreateInfo {
  unsigned width = 0;
  unsigned height = 0;
  unsigned depth = 1;
  unsigned levels = 1;

  VkFormat format = VK_FORMAT_UNDEFINED;
  VkImageType type = VK_IMAGE_TYPE_2D;

  unsigned layers = 1;

  VkImageUsageFlags usage = 0;
  VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
  VkImageCreateFlags flags = 0;

  VkImageLayout initial_layout = VK_IMAGE_LAYOUT_GENERAL;
  VkComponentMapping swizzle = {
      VK_COMPONENT_SWIZZLE_R,
      VK_COMPONENT_SWIZZLE_G,
      VK_COMPONENT_SWIZZLE_B,
      VK_COMPONENT_SWIZZLE_A,
  };

  static ImageCreateInfo RenderTarget(uint32_t width, uint32_t height, VkFormat format) {
    ImageCreateInfo info;
    info.width = width;
    info.height = height;
    info.depth = 1;
    info.levels = 1;
    info.format = format;
    info.type = VK_IMAGE_TYPE_2D;
    info.layers = 1;
    info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.flags = 0;
    info.initial_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    return info;
  }
};

class Image {
 public:
  Image(VkDevice device, VkImage image, VkImageView default_view, const ImageCreateInfo &info,
        VkImageViewType view_type);

  Image(Image &) = delete;
  Image(Image &&) = default;

  [[nodiscard]] ImageViewPtr GetView() { return view_; }

  [[nodiscard]] const ImageCreateInfo &GetCreateInfo() const { return info_; }

  [[nodiscard]] uint32_t GetWidth() const { return info_.width; }
  [[nodiscard]] uint32_t GetHeight() const { return info_.height; }

  [[nodiscard]] bool IsSwapchainImage() const { return swapchain_layout_ != VK_IMAGE_LAYOUT_UNDEFINED; }

  [[nodiscard]] VkImageLayout GetSwapchainLayout() const { return swapchain_layout_; }

  void SetSwapchainLayout(VkImageLayout layout) { swapchain_layout_ = layout; }

 private:
  VkDevice device_;

  const ImageCreateInfo info_;
  // TODO: transfer ownership of image to this class
  VkImage image_;
  ImageViewPtr view_;

  VkImageLayout swapchain_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
};
using ImagePtr = std::shared_ptr<Image>;

}  // namespace vre::rendering