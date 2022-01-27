#pragma once

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
  ImageView(VkImageView view, ImageViewCreateInfo &&info);

  ImageView(ImageView &) = delete;
  ImageView(ImageView &&) = delete;

  [[nodiscard]] VkFormat GetFormat() const { return info_.format; }

  [[nodiscard]] const Image &GetImage() const { return *info_.image; }

  [[nodiscard]] const ImageViewCreateInfo &GetCreateInfo() const { return info_; }

 private:
  VkImageView view_;
  const ImageViewCreateInfo info_;
};

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
};

class Image {
 public:
  Image(VkDevice *device, VkImage image, VkImageView default_view, const ImageCreateInfo &info, VkImageViewType view_type);

  Image(Image &) = delete;
  Image(Image &&) = delete;

  const ImageView &GetView() const { return *view_; }

  const ImageCreateInfo &GetCreateInfo() const { return info_; }

  bool IsSwapchainImage() const { return swapchain_layout != VK_IMAGE_LAYOUT_UNDEFINED; }

  VkImageLayout GetSwapchainLayout() const { return swapchain_layout; }

 private:
  const ImageCreateInfo info_;
  std::shared_ptr<ImageView> view_;

  VkImageLayout swapchain_layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

}  // namespace vre::rendering